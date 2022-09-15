#define _GNU_SOURCE
#include "compile.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

typedef enum { RDI, RSI, RBX, RBP, R12, R13, R14, R15, R8, R9, R10, R11, RAX, RCX, RDX, RSP, PLACEHOLDER_REG } reg_t;
typedef enum { REG, STACK, UNUSED } var_loc_t;
typedef enum { TEMPVAL, VARVAL } reg_use_t;
typedef enum { CONSTANT, REGVAL, REGOFFSET } asm_op_type_t;

const size_t MAX_VARIABLES = 26;
const size_t MAX_REGISTERS = 16;	// do not include placeholder
const char *REG_NAME_MAP[MAX_REGISTERS] = { "rdi", "rsi", "rbx", "rbp", "r12", "r13", "r14", "r15", "r8", "r9", "r10", "r11", "rax", "rcx", "rdx", "rsp"};
const size_t MAX_VAR_REGISTERS = 9;
const reg_t REGS_AVAILABLE_FOR_VARS[MAX_VAR_REGISTERS] = { RBX, RBP, R12, R13, R14, R15, R8, R9, R10 };
const reg_t DEFAULT_TEMPVAL_REGISTER = RDI;
const reg_t DEFAULT_TEMPVAL_REGISTER2 = RSI;
const reg_t DEFAULT_CLONE_REGISTER = R11;
const bool RESERVED_REGISTERS[] = { [RAX] = true, [RCX] = true, [RDX] = true, [RSP] = true, [R11] = true };
const char *STATEMENT_START_STR = "## === NEW STATEMENT ===";

typedef struct {
	reg_t id;
	bool reserved;  // if it's taken by a variable / or used by system
	size_t users;   // how many active uses so far
	bool is_used;	// if it's ever used
} reg_data_t;

typedef struct {
	reg_t reg;
	size_t key;
} reg_wrapper_t;

typedef struct {
	var_loc_t loc;
	union {
		reg_t id;
		size_t offset;
	} value;
} var_data_t;

typedef struct {
	asm_op_type_t type;
	union {
		int64_t num;
		reg_t reg;
		struct {
			reg_t reg;
			int64_t offset;
		} reg_offset;
	} value;
} asm_op_t;

// unique to a compilation request
typedef struct {
	reg_data_t reg_data[MAX_REGISTERS];
	var_data_t var_data[MAX_VARIABLES];
	size_t vars_in_regs;
	size_t vars_on_stack;
	size_t if_count;
	size_t while_count;
	size_t stack_height;	// not including vars_on_stack
	asm_op_t current_bound_var;
	asm_op_t current_clone_var;
	bool disable_intelli_swapping;
	size_t bin_op_recursion_depth;
	FILE *outputfile;
} state_t;

// necessary for variable counting (since we need sort capabilities)
typedef struct {
	char name;
	size_t count;
} var_counter_t;

//
//  REGISTER FUNCTIONS
//

void push(state_t *state, asm_op_t src);
void pop(state_t *state, asm_op_t dest);
static inline asm_op_t reg_op(reg_t reg);
static inline asm_op_t num_op(int64_t num);

static inline bool must_save_register(reg_data_t *rdmap, reg_t reg) {
	return rdmap[reg].users > 1;
}

static inline void release_register(state_t *state, reg_t reg) {
	assert(reg != RAX);	// to catch some errors
	fprintf(stderr, "releasing %s\n", REG_NAME_MAP[reg]);
	if (state->reg_data[reg].users-- > 1) {
		pop(state, reg_op(reg));
	}
}

// will error if called to allocate a varval when all var registers have been assigned
// caller should verify this condition
reg_t request_register(reg_data_t *rdmap, reg_use_t use) {
	switch (use) {
		case VARVAL: {
			for (size_t i = 0; i < MAX_VAR_REGISTERS; ++i) {
				reg_t reg = REGS_AVAILABLE_FOR_VARS[i];
				reg_data_t *data = &rdmap[reg];
				if (!data->reserved) {
					data->reserved = true;
					data->users = 1;
					data->is_used = true;
					return reg;
				}   
			}
		}
		case TEMPVAL: {
			for (reg_t reg = 0; reg < MAX_REGISTERS; ++reg) {
				reg_data_t *data = &rdmap[reg];
				if (!data->reserved && data->users == 0) {
					data->users++;
					data->is_used = true;
					return reg;
				}
			}
			if (rdmap[DEFAULT_TEMPVAL_REGISTER].users > rdmap[DEFAULT_TEMPVAL_REGISTER2].users) {
				rdmap[DEFAULT_TEMPVAL_REGISTER2].users++;
				rdmap[DEFAULT_TEMPVAL_REGISTER2].is_used = true;
				return DEFAULT_TEMPVAL_REGISTER2;
			} else {
				rdmap[DEFAULT_TEMPVAL_REGISTER].users++;
				rdmap[DEFAULT_TEMPVAL_REGISTER].is_used = true;
				return DEFAULT_TEMPVAL_REGISTER;
			}
			
		}
	}
	exit(15);   // should never reach here
}

static inline reg_t request_and_save_temp_register(state_t *state) {
	reg_t reg = request_register(state->reg_data, TEMPVAL);
	fprintf(stderr, "requesting %s\n", REG_NAME_MAP[reg]);
	if (must_save_register(state->reg_data, reg) ) {fprintf(stderr, "must push\n"); push(state, reg_op(reg));};
	return reg;
}

reg_t force_request_and_save_register(state_t *state, reg_t reg) {
	fprintf(stderr, "requesting %s\n", REG_NAME_MAP[reg]);
	state->reg_data[reg].users++;
	state->reg_data[reg].is_used = true;
	if (must_save_register(state->reg_data, reg) ) {fprintf(stderr, "must push\n"); push(state, reg_op(reg));};
	return reg;
}
//
//  VARIABLE ALLOCATION FUNCTIONS
//
//  Simple (but naive) algorithm: prioritize registers to variable with the most references (does NOT accurately consider while/if statements)

void count_variables(node_t *node, var_counter_t *map) {
	switch (node->type) {
		case SEQUENCE: {
			sequence_node_t *snode = (sequence_node_t*)node;
			for (size_t i = 0; i < snode->statement_count; ++i) {
				count_variables(snode->statements[i], map);
			}
			break;
		}
		case BINARY_OP:
			count_variables(((binary_node_t*)node)->left, map);
			count_variables(((binary_node_t*)node)->right, map);
			break;
		case PRINT:
			count_variables(((print_node_t*)node)->expr, map);
			break;
		case LET: {
			let_node_t *lnode = (let_node_t*)node;
			map[lnode->var - 'A'].count++;
			count_variables(lnode->value, map);
			break;
		}
		case IF: {
			if_node_t *inode = (if_node_t*)node;
			count_variables((node_t*)inode->condition, map);
			count_variables(inode->if_branch, map);
			if (inode->else_branch) count_variables(inode->else_branch, map);
			break;
		}
		case WHILE: {
			while_node_t *wnode = (while_node_t*)node;
			count_variables((node_t*)wnode->condition, map);
			count_variables(wnode->body, map);
			break;
		}
		case VAR:
			map[((var_node_t*)node)->name - 'A'].count++;
		case NUM:
			break;
	}
}

// flip the order because we want to sort in descending order
int sort_variable_countfn(var_counter_t *a, var_counter_t *b) {
	if (a->count > b->count) {
		return -1;
	} else if (a->count < b->count) {
		return 1;
	} else {
		return 0;
	}
}

void assign_variables(state_t *state, node_t *root) {
	var_counter_t vcmap[MAX_VARIABLES];
	for (char c = 'A'; c <= 'Z'; ++c) {
		vcmap[c - 'A'] = (var_counter_t) {
			.name = c,
			.count = 0
		};
	}
	count_variables(root, vcmap);
	// TESTING PURPOSES: DO NOT SORT
	qsort(vcmap, MAX_VARIABLES, sizeof(var_counter_t), (__compar_fn_t)sort_variable_countfn);

	reg_data_t *rdmap = state->reg_data;
	var_data_t *vdmap = state->var_data;

	for (size_t i = 0; i < MAX_VARIABLES; ++i) {
		var_counter_t data = vcmap[i];
		// TESTING PURPOSES: USE >= INSTEAD OF >
		if (data.count > 0) {
			fprintf(stderr, "[V] Assigning '%c' to ", data.name);
			if (state->vars_in_regs < MAX_VAR_REGISTERS) {
				reg_t reg = request_register(rdmap, VARVAL);
				state->vars_in_regs++;
				vdmap[data.name - 'A'] = (var_data_t) {
					.loc = REG,
					.value.id = reg
				};
				fprintf(stderr, "%s\n", REG_NAME_MAP[reg]);
			} else {
				vdmap[data.name - 'A'] = (var_data_t) {
					.loc = STACK,
					.value.offset = (state->vars_on_stack++) * sizeof(int64_t)
				};
				fprintf(stderr, "stack offset %zu\n", vdmap[data.name - 'A'].value.offset);
			}
		}
	}
}

var_data_t reassign_variable(state_t *state, char var, var_data_t data) {
	var_data_t old = state->var_data[var - 'A'];
	state->var_data[var - 'A'] = data;
	return old;
}

//
//  STATE FUNCTIONS
//

state_t *init_state() {
	state_t *state = calloc(1, sizeof(*state));
	for (size_t i = 0; i < MAX_REGISTERS; ++i) {
		state->reg_data[i].id = i;
		state->reg_data[i].reserved = RESERVED_REGISTERS[i];
	}
	for (size_t i = 0; i < MAX_VARIABLES; ++i) {
		state->var_data[i].loc = UNUSED;
	}
	return state;
}

void free_state(state_t *state) {
	if (state->outputfile) assert(fclose(state->outputfile) == 0);
	free(state);
}

//
//	ASM FUNCTIONS
//		These functions implement instruction validation
//

static inline asm_op_t reg_op(reg_t reg) {
	return (asm_op_t) {
		.type = REGVAL,
		.value.reg = reg
	};
}

static inline asm_op_t num_op(int64_t num) {
	return (asm_op_t) {
		.type = CONSTANT,
		.value.num = num
	};
}

static inline asm_op_t reg_offset_op(reg_t reg, size_t offset) {
	return (asm_op_t) {
		.type = REGOFFSET,
		.value.reg_offset = {
			.reg = reg,
			.offset = offset
		}
	};
}

static inline asm_op_t var_stack_op(state_t *state, size_t offset) {
	return reg_offset_op(RSP, offset + state->stack_height);
}

static inline asm_op_t var_op(state_t *state, char var) {
	var_data_t vdata = state->var_data[var - 'A'];
	if (vdata.loc == REG) {
		return reg_op(vdata.value.id);
	} else {
		return var_stack_op(state, vdata.value.offset);
	}
}

static inline bool same_asm_ops(asm_op_t a, asm_op_t b) {
	if (a.type != b.type) {
		return false;
	} else {
		switch (a.type) {
			case CONSTANT:
				return a.value.num == b.value.num;
			case REGVAL:
				return a.value.reg == b.value.reg;
			case REGOFFSET:
				return (a.value.reg_offset.reg == b.value.reg_offset.reg) && (a.value.reg_offset.offset == b.value.reg_offset.offset);
		}
	}
}

static inline bool can_represent_as_32bit(int64_t v) {
	int32_t trunc = (int32_t)v;
	return (int64_t)trunc == v;
}

static inline bool is_power_of_two(int64_t num) {
	return num != 0 && (num & (num - 1)) == 0;
}

static inline uint8_t compute_log_2(int64_t num) {
	for (uint8_t i = 1; ; ++i) {
		if (num & (int64_t)1 << i) {
			return i;
		}
	}
	return 0;	// should never happen (caller responsible for verifying is_power_of_two)
}

void mov(state_t *state, asm_op_t src, asm_op_t dest) {
	assert(dest.type != CONSTANT);

	if (same_asm_ops(src, dest)) return;
	// patch for where LET A = A + B + C would clone A into R11 but then move R11's value into A
	if (src.type == REGVAL && src.value.reg == DEFAULT_CLONE_REGISTER && same_asm_ops(state->current_bound_var, dest)) return;
	fprintf(state->outputfile, "\tmovq ");
	switch (src.type) {
		case CONSTANT:
			fprintf(state->outputfile, "$%" PRId64 ", ", src.value.num);
			break;
		case REGVAL:
			fprintf(state->outputfile, "%%%s, ", REG_NAME_MAP[src.value.reg]);
			break;
		case REGOFFSET:
			fprintf(state->outputfile, "%" PRId64 "(%%%s), ", src.value.reg_offset.offset, REG_NAME_MAP[src.value.reg_offset.reg]);
			break;
	}

	switch (dest.type) {
		case REGVAL:
			fprintf(state->outputfile, "%%%s\n", REG_NAME_MAP[dest.value.reg]);
			break;
		case REGOFFSET:
			if (src.type == REGOFFSET || (src.type == CONSTANT && !can_represent_as_32bit(src.value.num))) {
				fprintf(state->outputfile, "%%%s\n", REG_NAME_MAP[RCX]);
				mov(state, reg_op(RCX), dest);
			} else {
				fprintf(state->outputfile, "%" PRId64 "(%%%s)\n", dest.value.reg_offset.offset, REG_NAME_MAP[dest.value.reg_offset.reg]);
			}
		default: break;
	}
}

void add(state_t *state, asm_op_t src, asm_op_t dest) {
	assert(src.type != REGOFFSET || dest.type != REGOFFSET);
	assert(dest.type != CONSTANT);
	
	switch (src.type) {
		case CONSTANT: {
			int64_t num = src.value.num;
			if (can_represent_as_32bit(num)) {
				fprintf(state->outputfile, "\taddq $%" PRId64 ", ", num);
			} else {
				mov(state, num_op(num), reg_op(RCX));
				return add(state, reg_op(RCX), dest);
			}
			break;
		}
		case REGVAL:
			fprintf(state->outputfile, "\taddq %%%s, ", REG_NAME_MAP[src.value.reg]);
			break;
		case REGOFFSET:
			fprintf(state->outputfile, "\taddq %" PRId64 "(%%%s), ", src.value.reg_offset.offset, REG_NAME_MAP[src.value.reg_offset.reg]);
			break;
	}

	switch (dest.type) {
		case REGVAL:
			fprintf(state->outputfile, "%%%s\n", REG_NAME_MAP[dest.value.reg]);
			break;
		case REGOFFSET:
			fprintf(state->outputfile, "%" PRId64 "(%%%s)\n", dest.value.reg_offset.offset, REG_NAME_MAP[dest.value.reg_offset.reg]);
			break;
		default: break;
	}
}

void sub(state_t *state, asm_op_t src, asm_op_t dest) {
	assert(src.type != REGOFFSET || dest.type != REGOFFSET);
	assert(dest.type != CONSTANT);
	
	switch (src.type) {
		case CONSTANT: {
			int64_t num = src.value.num;
			if (can_represent_as_32bit(num)) {
				fprintf(state->outputfile, "\tsubq $%" PRId64 ", ", num);
			} else {
				mov(state, num_op(num), reg_op(RCX));
				return sub(state, reg_op(RCX), dest);
			}
			break;
		}
		case REGVAL:
			fprintf(state->outputfile, "\tsubq %%%s, ", REG_NAME_MAP[src.value.reg]);
			break;
		case REGOFFSET:
			fprintf(state->outputfile, "\tsubq %" PRId64 "(%%%s), ", src.value.reg_offset.offset, REG_NAME_MAP[src.value.reg_offset.reg]);
			break;
	}

	switch (dest.type) {
		case REGVAL:
			fprintf(state->outputfile, "%%%s\n", REG_NAME_MAP[dest.value.reg]);
			break;
		case REGOFFSET:
			fprintf(state->outputfile, "%" PRId64 "(%%%s)\n", dest.value.reg_offset.offset, REG_NAME_MAP[dest.value.reg_offset.reg]);
			break;
		default: break;
	}
}

// dest * src
void imul(state_t *state, asm_op_t src, asm_op_t dest) {
	assert(src.type != REGOFFSET || dest.type != REGOFFSET);
	assert(dest.type != CONSTANT && dest.type != REGOFFSET);
	
	switch (src.type) {
		case CONSTANT: {
			int64_t num = src.value.num;
			if (is_power_of_two(num)) {
				uint8_t shift_amt = compute_log_2(num);
				fprintf(state->outputfile, "\tshlq $%" PRIu8 ", ", shift_amt);
			} else {
				if (num == -1) {
					fprintf(state->outputfile, "\tnegq ");
				} else if (can_represent_as_32bit(num)) {
					fprintf(state->outputfile, "\timulq $%" PRId64 ", ", num);
				} else {
					mov(state, num_op(num), reg_op(RCX));
					return imul(state, reg_op(RCX), dest);
				}
			}		
			break;
		}
		case REGVAL:
			fprintf(state->outputfile, "\timulq %%%s, ", REG_NAME_MAP[src.value.reg]);
			break;
		case REGOFFSET:
			fprintf(state->outputfile, "\timulq %zu(%%%s), ", src.value.reg_offset.offset, REG_NAME_MAP[src.value.reg_offset.reg]);
			break;
	}

	fprintf(state->outputfile, "%%%s\n", REG_NAME_MAP[dest.value.reg]);
}

void idiv(state_t *state, asm_op_t src, asm_op_t dest, asm_op_t true_dest) {
	assert(src.type != REGOFFSET || dest.type != REGOFFSET);
	assert(dest.type != CONSTANT);

	mov(state, true_dest, reg_op(RAX));
	fprintf(state->outputfile, "\tcqo\n");
	
	switch (src.type) {
		case CONSTANT:
			mov(state, src, reg_op(RCX));
			fprintf(state->outputfile, "\tidivq %%%s\n", REG_NAME_MAP[RCX]);
			break;
		case REGVAL:
			fprintf(state->outputfile, "\tidivq %%%s\n", REG_NAME_MAP[src.value.reg]);
			break;
		case REGOFFSET:
			fprintf(state->outputfile, "\tidivq %zu(%%%s)\n", src.value.reg_offset.offset, REG_NAME_MAP[src.value.reg_offset.reg]);
			break;
		default: break;
	}
	if (dest.type == REGVAL && dest.value.reg == DEFAULT_CLONE_REGISTER && state->bin_op_recursion_depth == 1) {
		// optimization
		mov(state, reg_op(RAX), state->current_clone_var);
	} else {
		mov(state, reg_op(RAX), dest);
	}
}

// recall: src = right hand side (cannot have constant)
// dest = left hand side
void cmp(state_t *state, asm_op_t src, asm_op_t dest) {
	assert(src.type != REGOFFSET || dest.type != REGOFFSET);
	assert(src.type != CONSTANT);

	// dest = "left hand side"
	switch (dest.type) {
		case CONSTANT: {
			int64_t num = dest.value.num;
			if (can_represent_as_32bit(num)) {
				fprintf(state->outputfile, "\tcmpq $%" PRId64 ", ", num);
			} else {
				mov(state, dest, reg_op(RCX));
				fprintf(state->outputfile, "\tcmpq %%%s, ", REG_NAME_MAP[RCX]);
			}
			break;
		}
		case REGVAL: {
			fprintf(state->outputfile, "\tcmpq %%%s, ", REG_NAME_MAP[dest.value.reg]);
			break;
		}
		case REGOFFSET: {
			fprintf(state->outputfile, "\tcmpq %zu(%%%s), ", dest.value.reg_offset.offset, REG_NAME_MAP[dest.value.reg_offset.reg]);
			break;
		}
		default: break;
	}
	switch (src.type) {
		case REGVAL: {
			fprintf(state->outputfile, "%%%s\n", REG_NAME_MAP[src.value.reg]);
			break;
		}
		case REGOFFSET: {
			fprintf(state->outputfile, "%zu(%%%s)\n", src.value.reg_offset.offset, REG_NAME_MAP[src.value.reg_offset.reg]);
			break;
		}
		default: break;
	}
}

void push(state_t *state, asm_op_t src) {
	assert(src.type != CONSTANT);
	fprintf(state->outputfile, "\tpush ");
	state->stack_height += sizeof(int64_t);
	switch (src.type) {
		case REGVAL:
			fprintf(state->outputfile, "%%%s\n", REG_NAME_MAP[src.value.reg]);
			break;
		case REGOFFSET:
			fprintf(state->outputfile, "%zu(%%%s)\n", src.value.reg_offset.offset, REG_NAME_MAP[src.value.reg_offset.reg]);
			break;
		default: break;
	}
}

void pop(state_t *state, asm_op_t dest) {
	assert(dest.type != CONSTANT);
	fprintf(state->outputfile, "\tpop ");
	state->stack_height -= sizeof(int64_t);
	switch (dest.type) {
		case REGVAL:
			fprintf(state->outputfile, "%%%s\n", REG_NAME_MAP[dest.value.reg]);
			break;
		case REGOFFSET:
			fprintf(state->outputfile, "%zu(%%%s)\n", dest.value.reg_offset.offset, REG_NAME_MAP[dest.value.reg_offset.reg]);
			break;
		default: break;
	}
}

void save_caller_saved_registers(state_t *state) {
	reg_data_t *reg_data = state->reg_data;
	if (reg_data[R8].users) push(state, reg_op(R8));
	if (reg_data[R9].users) push(state, reg_op(R9));
	if (reg_data[R10].users) push(state, reg_op(R10));
	if (reg_data[R11].users) push(state, reg_op(R11));
}

// future/to-do: delete this if there are no other things after the print -- peek functionality?
void load_caller_saved_registers(state_t *state) {
	reg_data_t *reg_data = state->reg_data;
	if (reg_data[R11].users) pop(state, reg_op(R11));
	if (reg_data[R10].users) pop(state, reg_op(R10));
	if (reg_data[R9].users) pop(state, reg_op(R9));
	if (reg_data[R8].users) pop(state, reg_op(R8));
}

// should be called at the end (see main compile func)
static inline void save_callee_saved_registers(state_t *state) {
	reg_data_t *reg_data = state->reg_data;
	if (reg_data[RBX].is_used) push(state, reg_op(RBX));
	if (reg_data[RBP].is_used) push(state, reg_op(RBP));
	if (reg_data[R12].is_used) push(state, reg_op(R12));
	if (reg_data[R13].is_used) push(state, reg_op(R13));
	if (reg_data[R14].is_used) push(state, reg_op(R14));
	if (reg_data[R15].is_used) push(state, reg_op(R15));
}

static inline void load_callee_saved_registers(state_t *state) {
	reg_data_t *reg_data = state->reg_data;
	if (reg_data[R15].is_used) pop(state, reg_op(R15));
	if (reg_data[R14].is_used) pop(state, reg_op(R14));
	if (reg_data[R13].is_used) pop(state, reg_op(R13));
	if (reg_data[R12].is_used) pop(state, reg_op(R12));
	if (reg_data[RBP].is_used) pop(state, reg_op(RBP));
	if (reg_data[RBX].is_used) pop(state, reg_op(RBX));
}

//
//	MAIN
//

void perform_op(state_t *state, asm_op_t src, asm_op_t dest, asm_op_t true_dest, char *op) {
	switch (*op) {
		case '+': return add(state, src, dest);
		case '-': return sub(state, src, dest);
		case '*': return imul(state, src, dest);
		case '/': return idiv(state, src, dest, true_dest);
		default: break;	// ignore (probably a comparison, which we don't handle here)
	}
}

static inline bool is_div_chained_backward(binary_node_t *bnode, binary_node_t *previous) {
	return previous && previous->op == '/' && previous->left == (node_t*)bnode && previous->right->type != BINARY_OP;
}

//static inline bool is_div_chained_forward(binary_node_t *bnode, binary_node_t *child) {
//	return child && child->op == '/' && bnode->left == (node_t*)child && bnode->right->type != BINARY_OP;
//}

// only for num/var
bool does_non_binop_contain_var(node_t *node, char var) {
	return node->type == VAR && ((var_node_t*)node)->name == var;
}

bool does_binary_op_contain_var(binary_node_t *bnode, char var) {
	return (bnode->left->type == BINARY_OP ? does_binary_op_contain_var((binary_node_t*)bnode->left, var) : does_non_binop_contain_var(bnode->left, var))
		|| (bnode->right->type == BINARY_OP ? does_binary_op_contain_var((binary_node_t*)bnode->right, var) : does_non_binop_contain_var(bnode->right, var));
}

// for special let statements that are chained

bool is_chained_bnode_let(binary_node_t *bnode, char var) {
	bool left_condition = (bnode->left->type == BINARY_OP && is_chained_bnode_let((binary_node_t*)bnode->left, var))
							|| (bnode->left->type == VAR && ((var_node_t*)bnode->left)->name == var);
	if (!left_condition) return false;
	
	bool right_condition = (bnode->right->type == BINARY_OP && !does_binary_op_contain_var((binary_node_t*)bnode->right, var))
							|| (bnode->right->type != BINARY_OP && !does_non_binop_contain_var(bnode->right, var));
	//print_ast(bnode->right);
	//printf("%c\n", var);
	//printf("bin? %d\n", bnode->right->type);
	//printf("right condition: %d\n", right_condition);
	return right_condition;
}

void switch_bnode(binary_node_t *b) {
	node_t *old_left = b->left;
	b->left = b->right;
	b->right = old_left;
	if (b->op == '<') {
		b->op = '>';
	} else if (b->op == '>') {
		b->op = '<';
	}
}

static inline reg_t compile_bin_op_ret_wrapper(state_t *state, reg_t reg) {
	state->bin_op_recursion_depth--;
	return reg;
}

// Caller responsible for releasing register
// to anyone who has to read this: i'm sorry, but generating semi-optimal asm is complicated :(
reg_t compile_binary_op(binary_node_t *bnode, binary_node_t *previous, state_t *state, asm_op_t *first_dest) {
	node_t *left = bnode->left;
	node_t *right = bnode->right;

	state->bin_op_recursion_depth++;

	// Swap order for special cases (allows us to save/reuse registers and pass them on)
	// CHECK IN THIS ORDER
	// 1) -1 * binop/var --> bin/op * -1 [so we can perform neg]
	// 2) power of 2 * var/bin-op ---> var/bin-op * power of 2
	// 3) var/num +/* BINOP --->  BINOP +/* num [if num-32-bit, we save a register]
	// 4) var +/* num --> num +/* var [so num is directly loaded to target register, avoid possible rcx move]
	//		possible collision with (2)
	
	fprintf(stderr, "can check? %d\n", !state->disable_intelli_swapping);
	
	if (!state->disable_intelli_swapping) {
		bool do_swap = false;
		// use else if (we don't want to accidentaly reswap)
		if (left->type == NUM && ((num_node_t*)left)->value == -1 && right->type != NUM && bnode->op == '*') {
			do_swap = true;
		} else if (left->type == NUM && bnode->op == '*' && is_power_of_two(((num_node_t*)left)->value)) {
			do_swap = true;
		} else if (left->type != BINARY_OP && right->type == BINARY_OP && (bnode->op == '+' || bnode->op == '*')) {
			do_swap = true;
		} else if (left->type == VAR && right->type == NUM && (bnode->op == '+' || bnode->op == '*')) {
			if (!(bnode->op == '*' && is_power_of_two(((num_node_t*)right)->value))) {
				do_swap = true;
			}
		}
		if (do_swap) {
			switch_bnode(bnode);
			left = bnode->left;
			right = bnode->right;
		}
	}

	// We don't handle num OP num due to that being handled by constant folding
	if (left->type == NUM && right->type == VAR) {
		num_node_t *num = (num_node_t*)left;
		var_node_t *var = (var_node_t*)right;
		if (bnode->op == '/' && is_div_chained_backward(bnode, previous)) {
			perform_op(state, var_op(state, var->name), reg_op(RAX), num_op(num->value), &bnode->op);
			return compile_bin_op_ret_wrapper(state, RAX);
		} else {
			reg_t reg = first_dest ? PLACEHOLDER_REG : request_and_save_temp_register(state);
			if (bnode->op != '/') {
				mov(state, num_op(num->value), first_dest ? *first_dest : reg_op(reg));
			}
			perform_op(state, var_op(state, var->name), first_dest ? *first_dest : reg_op(reg), num_op(num->value), &bnode->op);
			return compile_bin_op_ret_wrapper(state, reg);
		}
	} else if (left->type == VAR && right->type == NUM) {
		var_node_t *var = (var_node_t*)left;
		num_node_t *num = (num_node_t*)right;
		if (bnode->op == '/' && is_div_chained_backward(bnode, previous)) {
			perform_op(state, num_op(num->value), reg_op(RAX), var_op(state, var->name), &bnode->op);
			return compile_bin_op_ret_wrapper(state, RAX);
		} else {
			reg_t reg = first_dest ? PLACEHOLDER_REG : request_and_save_temp_register(state);
			if (bnode->op != '/') {
				mov(state, var_op(state, var->name), first_dest ? *first_dest : reg_op(reg));
			}
			perform_op(state, num_op(num->value), first_dest ? *first_dest : reg_op(reg), var_op(state, var->name), &bnode->op);
			return compile_bin_op_ret_wrapper(state, reg);
		}
	} else if (left->type == VAR && right->type == VAR) {
		var_node_t *var = (var_node_t*)left;
		var_node_t *var2 = (var_node_t*)right;
		if (bnode->op == '/' && is_div_chained_backward(bnode, previous)) {
			perform_op(state, var_op(state, var2->name), reg_op(RAX), var_op(state, var->name), &bnode->op);
			return compile_bin_op_ret_wrapper(state, RAX);
		} else {
			reg_t reg = first_dest ? PLACEHOLDER_REG : request_and_save_temp_register(state);
			if (bnode->op != '/') {
				mov(state, var_op(state, var->name), first_dest ? *first_dest : reg_op(reg));
			}
			perform_op(state, var_op(state, var2->name), first_dest ? *first_dest : reg_op(reg), var_op(state, var->name), &bnode->op);
			return compile_bin_op_ret_wrapper(state, reg);
		}
	} else if (left->type == BINARY_OP && right->type == NUM) {
		num_node_t *num = (num_node_t*)right;
		reg_t reg = compile_binary_op((binary_node_t*)left, bnode, state, first_dest);
		if (bnode->op == '/' && is_div_chained_backward(bnode, previous)) {	
			perform_op(state, num_op(num->value), reg_op(RAX), first_dest ? *first_dest : reg_op(reg), &bnode->op);
			if (reg != RAX && reg != PLACEHOLDER_REG) release_register(state, reg);
			return compile_bin_op_ret_wrapper(state, RAX);
		} else if (reg == RAX) {	// no longer div chained; was division
			reg_t new_reg = first_dest ? PLACEHOLDER_REG : request_and_save_temp_register(state);
			perform_op(state, num_op(num->value), first_dest ? *first_dest : reg_op(new_reg), reg_op(RAX), &bnode->op);
			return compile_bin_op_ret_wrapper(state, new_reg);
		} else {
			perform_op(state, num_op(num->value), first_dest ? *first_dest : reg_op(reg), first_dest ? *first_dest : reg_op(reg), &bnode->op);
			return compile_bin_op_ret_wrapper(state, reg);
		}
	} else if (left->type == BINARY_OP && right->type == VAR) {
		var_node_t *var = (var_node_t*)right;
		reg_t reg = compile_binary_op((binary_node_t*)left, bnode, state, first_dest);
		if (bnode->op == '/' && is_div_chained_backward(bnode, previous)) {
			perform_op(state, var_op(state, var->name), reg_op(RAX), first_dest ? *first_dest : reg_op(reg), &bnode->op);
			if (reg != RAX && reg != PLACEHOLDER_REG) release_register(state, reg);
			return compile_bin_op_ret_wrapper(state, RAX);
		} else if (reg == RAX) {	// no longer div chained; was division
			reg_t new_reg = first_dest ? PLACEHOLDER_REG : request_and_save_temp_register(state);
			perform_op(state, var_op(state, var->name), first_dest ? *first_dest : reg_op(new_reg), reg_op(RAX), &bnode->op);
			return compile_bin_op_ret_wrapper(state, new_reg);
		} else {
			perform_op(state, var_op(state, var->name), first_dest ? *first_dest : reg_op(reg), first_dest ? *first_dest : reg_op(reg), &bnode->op);
			return compile_bin_op_ret_wrapper(state, reg);
		}
	} else if (left->type == NUM && right->type == BINARY_OP) {
		num_node_t *num = (num_node_t*)left;
		if (bnode->op == '/' && is_div_chained_backward(bnode, previous)) {
			reg_t temp_reg = compile_binary_op((binary_node_t*)right, bnode, state, NULL);
			perform_op(state, reg_op(temp_reg), reg_op(RAX), first_dest ? *first_dest : num_op(num->value), &bnode->op);
			if (temp_reg != RAX && temp_reg != PLACEHOLDER_REG) release_register(state, temp_reg);
			return compile_bin_op_ret_wrapper(state, RAX);
		} else {	// not possible for temp_reg to be RAX b/c is_div_chained not met
			reg_t reg = first_dest ? PLACEHOLDER_REG : request_and_save_temp_register(state);
			reg_t temp_reg = compile_binary_op((binary_node_t*)right, bnode, state, NULL);
			if (bnode->op != '/') {
				mov(state, num_op(num->value), first_dest ? *first_dest : reg_op(reg));
			}
			perform_op(state, reg_op(temp_reg), first_dest ? *first_dest : reg_op(reg), num_op(num->value), &bnode->op);
			release_register(state, temp_reg);
			return compile_bin_op_ret_wrapper(state, reg);
		}
	} else if (left->type == VAR && right->type == BINARY_OP) {
		var_node_t *var = (var_node_t*)left;
		if (bnode->op == '/' && is_div_chained_backward(bnode, previous)) {
			reg_t temp_reg = compile_binary_op((binary_node_t*)right, bnode, state, NULL);
			perform_op(state, reg_op(temp_reg), reg_op(RAX), first_dest ? *first_dest : var_op(state, var->name), &bnode->op);
			if (temp_reg != RAX && temp_reg != PLACEHOLDER_REG) release_register(state, temp_reg);
			return compile_bin_op_ret_wrapper(state, RAX);
		} else {	// not possible for temp_reg to be RAX b/c is_div_chained not met
			reg_t reg = first_dest ? PLACEHOLDER_REG : request_and_save_temp_register(state);
			reg_t temp_reg = compile_binary_op((binary_node_t*)right, bnode, state, NULL);
			if (bnode->op != '/') {
				mov(state, var_op(state, var->name), first_dest ? *first_dest : reg_op(reg));
			}
			perform_op(state, reg_op(temp_reg), first_dest ? *first_dest : reg_op(reg), var_op(state, var->name), &bnode->op);
			release_register(state, temp_reg);
			return compile_bin_op_ret_wrapper(state, reg);
		}
	} else {
		// bin-op OP bin-op
		reg_t left_reg = compile_binary_op((binary_node_t*)left, bnode, state, first_dest);
		reg_t right_reg = compile_binary_op((binary_node_t*)right, bnode, state, NULL);
		if (bnode->op == '/' && is_div_chained_backward(bnode, previous)) {
			perform_op(state, reg_op(right_reg), reg_op(RAX), first_dest ? *first_dest : reg_op(left_reg), &bnode->op);	
			if (right_reg != RAX && right_reg != PLACEHOLDER_REG) release_register(state, right_reg);
			if (left_reg != RAX && left_reg != PLACEHOLDER_REG) release_register(state, left_reg);
			return compile_bin_op_ret_wrapper(state, RAX);
		} else if (left_reg == RAX) {
			reg_t reg = first_dest ? PLACEHOLDER_REG : request_and_save_temp_register(state);
			perform_op(state, reg_op(right_reg), first_dest ? *first_dest : reg_op(reg), reg_op(RAX), &bnode->op);
			if (right_reg != RAX && right_reg != PLACEHOLDER_REG) release_register(state, right_reg);
			return compile_bin_op_ret_wrapper(state, reg);
		} else {
			//fprintf(stderr, "bin op OP bin op / no chain\n");
			perform_op(state, reg_op(right_reg), first_dest ? *first_dest : reg_op(left_reg), first_dest ? *first_dest : reg_op(left_reg), &bnode->op);
			if (right_reg != RAX && right_reg != PLACEHOLDER_REG) release_register(state, right_reg);
			return compile_bin_op_ret_wrapper(state, left_reg);
		}
	}
}

// assume both are NOT constants
// note: there are two cases if src & dest are both constants for a while loop:
// if it is always false, the optimizer removes it entirely
// if it is always true, the optimizer leaves as it, so we know that constant COMP constant is always true
// we handle the infinite case in the main loop (not here), so we assume that if we are dealing w/ a constant,
// there is only one, and we choose to place it in the source for ease
void compile_condition(binary_node_t *bnode, state_t *state) {
	node_t *left = bnode->left;
	node_t *right = bnode->right;

	if (left->type == NUM && right->type == VAR) {
		cmp(state, var_op(state, ((var_node_t*)right)->name), num_op(((num_node_t*)left)->value));
	} else if (left->type == VAR && right->type == NUM) {
		switch_bnode(bnode);
		compile_condition(bnode, state);
	} else if (left->type == VAR && right->type == VAR) {
		char lname = ((var_node_t*)left)->name;
		char rname = ((var_node_t*)right)->name;
		var_data_t ldata = state->var_data[lname - 'A'];
		var_data_t rdata = state->var_data[rname - 'A'];
		if (ldata.loc == STACK && rdata.loc == STACK) {
			mov(state, var_op(state, lname), reg_op(RCX));
			cmp(state, var_op(state, rname), reg_op(RCX));
		} else {
			cmp(state, var_op(state, rname), var_op(state, lname));
		}
	} else if (left->type == BINARY_OP && right->type == NUM) {
		switch_bnode(bnode);
		compile_condition(bnode, state);
	} else if (left->type == NUM && right->type == BINARY_OP) {
		reg_t reg = compile_binary_op((binary_node_t*)right, NULL, state, NULL);
		cmp(state, reg_op(reg), num_op(((num_node_t*)left)->value));
		release_register(state, reg);
	} else if (left->type == BINARY_OP && right->type == VAR) {
		reg_t reg = compile_binary_op((binary_node_t*)left, NULL, state, NULL);
		cmp(state, var_op(state, ((var_node_t*)right)->name), reg_op(reg));
		release_register(state, reg);
	} else if (left->type == VAR && right->type == BINARY_OP) {
		switch_bnode(bnode);
		compile_condition(bnode, state);
	} else {
		// bnode CMP bnode
		assert("turn on all optimizations!" && left->type == BINARY_OP && right->type == BINARY_OP);
		reg_t lreg = compile_binary_op((binary_node_t*)left, NULL, state, NULL);
		reg_t rreg = compile_binary_op((binary_node_t*)right, NULL, state, NULL);
		cmp(state, reg_op(rreg), reg_op(lreg));
		release_register(state, rreg);
		release_register(state, lreg);
	}
}

// possibility for improvement PRINT X + X <-- just do add $rdi, $rdi as last step (no need to take from source)
// movq something, $[read only]; $[read only], something?
// ... like r11? which is read only
// LET T = (((N + T) - ((Z + (C * (L - H))))) / X)
// test the bswap prog

bool compile(node_t *node, state_t *state) {
	fprintf(state->outputfile, "%s\n", STATEMENT_START_STR);
	switch (node->type) {
		case SEQUENCE: {
			sequence_node_t *snode = (sequence_node_t*)node;
			for (size_t i = 0; i < snode->statement_count; ++i) {
				int result = compile(snode->statements[i], state);
				if (!result) return false;
			}
			break;
		}
		case PRINT: {
			fprintf(state->outputfile, "# printing\n");
			print_node_t *pnode = (print_node_t*)node;
			node_t *expr = pnode->expr;
			
			switch (expr->type) {
				case NUM: {
					force_request_and_save_register(state, RDI);
					mov(state, num_op(((num_node_t*)expr)->value), reg_op(RDI));
					break;
				}
				case VAR: {
					force_request_and_save_register(state, RDI);
					var_data_t vdata = state->var_data[((var_node_t*)expr)->name - 'A'];
					if (vdata.loc == REG) {
						mov(state, reg_op(vdata.value.id), reg_op(RDI));
					} else {
						mov(state, var_stack_op(state, vdata.value.offset), reg_op(RDI));
					}
					break;
				}
				case BINARY_OP: {
					reg_t reg = compile_binary_op((binary_node_t*)expr, NULL, state, NULL);
					assert(reg == RDI);
					break;
				}
				default: break;
			}
			save_caller_saved_registers(state);
			fprintf(state->outputfile, "\tcall print_int\n");
			load_caller_saved_registers(state);
			release_register(state, RDI);
			
			break;
		}
		case LET: {
			let_node_t *lnode = (let_node_t*)node;
			node_t *expr = lnode->value;
			char var_name = ((var_node_t*)lnode)->name;
			var_data_t data = state->var_data[var_name - 'A'];

			fprintf(state->outputfile, "# Assigning to: '%c'\n", var_name);

			switch (expr->type) {
				case NUM:
					mov(state, num_op(((num_node_t*)expr)->value), var_op(state, var_name));
					break;
				case VAR:
					mov(state, var_op(state, ((var_node_t*)expr)->name), var_op(state, var_name));
					break;
				// indeed... this is complicated to try to obtain (semi)-optimal asm
				case BINARY_OP: {
					binary_node_t *bnode = (binary_node_t*)expr;
					if (does_binary_op_contain_var(bnode, var_name)) {
						// swap for efficiency (less register usage)
						if ((bnode->op == '+' || bnode->op == '*') &&
							((bnode->left->type == NUM && bnode->right->type == VAR)
							|| (bnode->left->type == VAR && bnode->right->type == VAR && ((var_node_t*)bnode->right)->name == var_name))) {
							node_t *temp = bnode->left;
							bnode->left = bnode->right;
							bnode->right = temp;
						}

						// special case for: var OP num and num OP var, e.g. a = a + 1
						if (bnode->left->type == VAR && bnode->right->type == NUM) {
							var_node_t *vnode = (var_node_t*)bnode->left;
							perform_op(state, num_op(((num_node_t*)bnode->right)->value), var_op(state, vnode->name), var_op(state, vnode->name), &bnode->op);
						} else if (bnode->left->type == NUM && bnode->right->type == VAR) {
							reg_t temp = request_and_save_temp_register(state);
							mov(state, num_op(((num_node_t*)bnode->left)->value), reg_op(temp));
							asm_op_t dest = var_op(state, ((var_node_t*)bnode->right)->name);
							perform_op(state, reg_op(temp), dest, dest, &bnode->op);
							release_register(state, temp);
						} else if (bnode->left->type == VAR && bnode->right->type == VAR && state->var_data[((var_node_t*)bnode->left)->name - 'A'].loc == REG) {
							var_node_t *vnode = (var_node_t*)bnode->left;
							perform_op(state, var_op(state, ((var_node_t*)bnode->right)->name), var_op(state, vnode->name), var_op(state, vnode->name), &bnode->op);
						} else {
							if (data.loc == STACK) {
								state->current_clone_var = var_op(state, var_name);
								asm_op_t dest_op = reg_op(DEFAULT_CLONE_REGISTER);
								compile_binary_op(bnode, NULL, state, &dest_op);
								if (bnode->op != '/') mov(state, dest_op, var_op(state, var_name));
								state->current_clone_var = reg_op(PLACEHOLDER_REG);
							} else {
								if (is_chained_bnode_let(bnode, var_name)) {
									// no need to bind
									asm_op_t asm_op = var_op(state, var_name);
									state->disable_intelli_swapping = true;
									compile_binary_op(bnode, NULL, state, &asm_op);
									state->disable_intelli_swapping = false;
								} else {
									// make temp; bind; execute as overwriting; and unbind
									asm_op_t dest_op = var_op(state, var_name);
									mov(state, var_op(state, var_name), reg_op(DEFAULT_CLONE_REGISTER));

									var_data_t old_data = reassign_variable(state, var_name, (var_data_t) {
										.loc = REG,
										.value.id = DEFAULT_CLONE_REGISTER
									});
									state->current_bound_var = dest_op;

									compile_binary_op(bnode, NULL, state, &dest_op);
									reassign_variable(state, var_name, old_data);

									state->current_bound_var = reg_op(PLACEHOLDER_REG);
								}	
							}
						}
					} else {
						// just overwrite the var
						if (data.loc == REG) {
							asm_op_t asm_op = var_op(state, var_name);
							compile_binary_op(bnode, NULL, state, &asm_op);
						
						} else {
							// possibility we add from a stack loc
							state->current_clone_var = var_op(state, var_name);
							asm_op_t asm_op = reg_op(DEFAULT_CLONE_REGISTER);
							compile_binary_op(bnode, NULL, state, &asm_op);
							if (bnode->op != '/') mov(state, asm_op, var_op(state, var_name));
							state->current_clone_var = reg_op(PLACEHOLDER_REG);
						}
					}
				}
				default: break;
			}
			break;
		}
		case WHILE: {
			while_node_t *wnode = (while_node_t*)node;
			binary_node_t *condition = (binary_node_t*)wnode->condition;
			size_t w_count = state->while_count++;
			fprintf(state->outputfile, "WHILE_%zu_START:\n", w_count);

			if (condition->left->type == NUM && condition->right->type == NUM) {
				// infinite loop
				bool result = compile(wnode->body, state);
				if (!result) return false;

				fprintf(state->outputfile, "\tjmp WHILE_%zu_START\n", w_count);
			} else {
				compile_condition(condition, state);
				switch (condition->op) {
					case '<':
						fprintf(state->outputfile, "\tjle WHILE_%zu_END\n", w_count);
						break;
					case '>':
						fprintf(state->outputfile, "\tjge WHILE_%zu_END\n", w_count);
						break;
					case '=':
						fprintf(state->outputfile, "\tjne WHILE_%zu_END\n", w_count);
						break;
					default: return false;
				}

				bool result = compile(wnode->body, state);
				if (!result) return false;

				fprintf(state->outputfile, "\tjmp WHILE_%zu_START\n", w_count);
				fprintf(state->outputfile, "WHILE_%zu_END:\n", w_count);
			}

			break;
		}
		case IF: {
			if_node_t *inode = (if_node_t*)node;
			binary_node_t *condition = (binary_node_t*)inode->condition;
			size_t if_count = state->if_count++;

			compile_condition(condition, state);

			switch (condition->op) {
				case '<':
					fprintf(state->outputfile, "\tjle IF_%zu_END\n", if_count);
					break;
				case '>':
					fprintf(state->outputfile, "\tjge IF_%zu_END\n", if_count);
					break;
				case '=':
					fprintf(state->outputfile, "\tjne IF_%zu_END\n", if_count);
					break;
				default: return false;
			}

			bool result = compile(inode->if_branch, state);
			if (!result) return false;

			if (inode->else_branch) {
				fprintf(state->outputfile, "\tjmp IF_%zu_ELSE_END\n", if_count);
				fprintf(state->outputfile, "IF_%zu_END:\n", if_count);
				result = compile(inode->else_branch, state);
				if (!result) return false;
				fprintf(state->outputfile, "IF_%zu_ELSE_END:\n", if_count);
			} else {
				fprintf(state->outputfile, "IF_%zu_END:\n", if_count);
			}
			break;
		}
		default: exit(99);
	}

	return true;
}

void post_process(state_t *state, FILE *read_from) {
	
	save_callee_saved_registers(state);
	if (state->vars_on_stack > 0) {
		sub(state, num_op(state->vars_on_stack * sizeof(int64_t)), reg_op(RSP));
	}

	char *lineptr = NULL;
	size_t buffer_size;
	assert(fseek(read_from, 0, SEEK_SET) == 0);
	while (getline(&lineptr, &buffer_size, read_from) >= 0) {
		fprintf(state->outputfile, "%s", lineptr);
		free(lineptr);
		lineptr = NULL;
	}
	free(lineptr);	// must free even if get line failed (per docs)

	if (state->vars_on_stack > 0) {
		add(state, num_op(state->vars_on_stack * sizeof(int64_t)), reg_op(RSP));
	}
	load_callee_saved_registers(state);	
}

bool compile_ast(node_t *node) {
	if (!node) return true;

	char template[] = "/tmp/compile_XXXXXX";
	int temp_fd = mkstemp(template);
	assert(temp_fd >= 0);

	state_t *state = init_state();
	FILE *temp_file = fdopen(temp_fd, "w+");
	state->outputfile = temp_file;
	assert(state->outputfile != NULL);

	assign_variables(state, node);
	bool result = compile(node, state);
	if (!result) return result;

	state->outputfile = stdout;
	post_process(state, temp_file);
	state->outputfile = temp_file;	 // so that it can be freed
	free_state(state);
	assert(unlink(template) == 0);
	
	return result;
}
