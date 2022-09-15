#include "ast_optimizer.h"
#include "queue.h"
#include <stdio.h>
#include <string.h>

/**
 * CONSTANT FOLDING ALGORITHM
 * 
 * - Find all the statements (traverse the AST)
 * - If a statement includes a binary op field,
 * -    perform the binary op condensing algorithm
 * -    replace the binary op field with the new value
 * -    destroy the old binary op object
 * 
 * // Returns either a num value, variable, or binary op
 * - Binary op condensing algorithm:
 * -    CHECKING ALGORITHM:
 * -        1) [X] ANY ORDER: num OP num --> evaluate
 * -        2) [X] SPECIFIC ORDER: 1 * [var/bin-op] OR [var/bin-op] * 1 OR [var/bin-op] / 1 --> return var/bin-op
 * -						   	   0 + [var/bin-op] OR [var/bin-op] + 0 OR [var/bin-op] - 0
 * -		 					   var - var OR 
 * -							   [any order] (-1 * VAR) + var OR
 * -							   (var / -1) + var
 * -							   (num/binop / -1) --> num/binop * -1
 * -        3) [X] ANY ORDER: [binary op (+,-)] (+,-) num --> add num to num of binary op
 * -			only exception: num - [bin/var +/- num] b/c this results in negating bin/var if we fold
 * -        4) [X] ANY ORDER: [binary op (*)] * num --> multiply num to num of binary op
 * -        5) [X] SPECIFIC ORDER: [binary op (/)] / num --> [inner var/bin-op] / (num * num)
 * -            refer to (divison is floor division): https://janmr.com/blog/2009/09/useful-properties-of-the-floor-and-ceil-functions/
 * -        6) [X] COMPARISON OP: return bin-op (do not evaluate)
 * -            this should be the first bin-op (no bin-op children should be comparisons)
 * -		7) [X] BINARY_OP W/ BINARY_OP:
 * -			ANY ORDER: (X +/- num) [+] (Y +/- num) --> (X + Y) + adjusted_num
 * -							ex: (X - num) + (Y - num) --> (X + Y) + (adjusted num)
 * -								(num - X) + (num - Y)
 * -								(num - X) + (Y - num)
 * -			SPECIFIC: 	(X [] num) - (Y [] num) --> (X - Y) + aa_num
 * -					  	(X [] num) - (num [] Y) --> (X + aa_num) - Y
 * -
 * -					  	(num [] X) - (Y [] num) --> (aa_num [] X) - Y
 * -						(num [] X) - (num [] Y)	--> ((num1 - num2) [] X) +/- Y
 * -    
 * -    Wrapper class: { parent, side_of_parent_bin_op, bin_op } -- we use this for the children
 * -    We perform a breadth-first traversal of a binary op:
 * -        Construct queue
 * -        Add original binary op to queue
 * -    While Q not empty:
 * -        N := Q[current_idx++] (note: we do not evaluate yet)
 * -        IF N.LEFT is binary op, add it to the queue
 * -        IF N.RIGHT is binary op, add it to the queue
 * -    Now, we traverse the queue backwards:
 * -        Steps (1) - (2) are straightforward
 * -        Steps (3) - (5) require ensuring that the bin-op is of depth 1 (so has no child bin-ops)
 * -        
 * -        IF WE MAKE A CHANGE:
 * -            Construct new node if necessary (or reuse an old one)
 * -            Destroy the old node (and its wrapper)
 * -            Set the appropriate parent's left/right side as needed
 * -            IF THE NODE is the root node, don't set the parent, just return it
 * -            
 * - NOTES:
 *      cannot fold ( / ) * or ( * ) / 
 *          e.g. (3 / i) * 3 --> if i = 4, we get 0 * 3, but folding would yield 9 / 4 = 2
 *          e.g. (i / 3) * 3 --> if i = 5, we get 1 * 3, but folding would yield 5
 *          e.g. (i * 4) / 5 --> if i = 22, we get 17, but folding would yield 22 * 0
 *          ... can come up w/ more examples if needed
 *      [(3 / i) + 3] + 10
 */

typedef enum { LEFT, RIGHT } bnode_side_t;

typedef struct {
	binary_node_t *parent;
	bnode_side_t side;  // 0 = left, 1 = right
	node_t *value;
} bnode_wrapper_t;

bnode_wrapper_t *create_bnode_wrapper(binary_node_t *parent, bnode_side_t side, binary_node_t *value) {
	bnode_wrapper_t *w = malloc(sizeof(*w));
	w->parent = parent;
	w->side = side;
	w->value = (node_t*)value;
	return w;
}

void add_binary_op_children(bnode_wrapper_t *w, queue_t *q) {
	binary_node_t *b = (binary_node_t*)w->value;
	if (b->right->type == BINARY_OP) {
		queue_add_to_back(q, create_bnode_wrapper(b, RIGHT, (binary_node_t*)b->right));
	}
	if (b->left->type == BINARY_OP) {
		queue_add_to_back(q, create_bnode_wrapper(b, LEFT, (binary_node_t*)b->left));
	}
}

void replace_node(bnode_wrapper_t *w, node_t *n) {
	w->value = n;
	if (!w->parent) return;

	if (w->side == LEFT) {
		w->parent->left = n;
	} else {
		w->parent->right = n;
	}
}

static inline int64_t perform_op(int64_t a, int64_t b, char op) {
	fprintf(stderr, "[M] Computing: %" PRId64 " %c %" PRId64 "\n", a, op, b);
	switch (op) {
		case '+': return a + b;
		case '-': return a - b;
		case '*': return a * b;
		case '/': return a / b;
		default: exit(1);   // should not happen
	}
}

static inline bool perform_comp(int64_t a, int64_t b, char op) {
	fprintf(stderr, "[C] Comparing: %" PRId64 " %c %" PRId64 "\n", a, op, b);
	switch (op) {
		case '<': return a < b;
		case '=': return a == b;
		case '>': return a > b;
		default: exit(2);
	}
}

// assume only one num
static inline int64_t get_num_of_binop(binary_node_t *b) {
	if (b->left->type == NUM) {
		return ((num_node_t*)b->left)->value;
	} else {
		return ((num_node_t*)b->right)->value;
	}
}

// assume only one num
// adjusted: a - 5 --> -5; 5 - a --> 5
static inline int64_t get_adjusted_num_of_binop(binary_node_t *b) {
	if (b->left->type == NUM) {
		return ((num_node_t*)b->left)->value;
	} else {
		return (b->op == '-' ? -1 : 1) * ((num_node_t*)b->right)->value;
	}
}

// assume 1 num and something else
static inline node_t *get_non_num_of_binop(binary_node_t *b) {
	if (b->left->type == NUM) {
		return b->right;
	} else {
		return b->left;
	}
}

// assume only one num 
static inline void modify_num_of_binop(binary_node_t *b, int64_t num) {
	if (b->left->type == NUM) {
		((num_node_t*)b->left)->value = num;
	} else {
		((num_node_t*)b->right)->value = num;
	}
}

static inline void replace_num_of_binop(binary_node_t *b, node_t *n) {
	if (b->left->type == NUM) {
		free_ast(b->left);
		b->left = n;
	} else {
		free_ast(b->right);
		b->right = n;
	}
}

static inline bool binop_has_num(binary_node_t *b) {
	return (b->left->type == NUM || b->right->type == NUM);
}

static inline bool binop_has_var(binary_node_t *b) {
	return (b->left->type == VAR || b->right->type == VAR);
}

binary_node_t *merge_binop_and_num(bnode_wrapper_t *w, bnode_side_t side_of_num) {
	binary_node_t *bnode = (binary_node_t*)w->value;
	binary_node_t *bchild = (binary_node_t*)(side_of_num == LEFT ? bnode->right : bnode->left);
	num_node_t *bnum = (num_node_t*)(side_of_num == LEFT ? bnode->left : bnode->right);

	if (bnode->op == '+' && bchild->op == '+') {
		modify_num_of_binop(bchild, bnum->value + get_num_of_binop(bchild));
	} else if (bnode->op == '*' && bchild->op == '*') {
		modify_num_of_binop(bchild, bnum->value * get_num_of_binop(bchild));
	} else if ((bnode->op == '+' || bnode->op == '-') && (bchild->op == '+' || bchild->op == '-')) {
		int64_t result = get_adjusted_num_of_binop(bnode) + get_adjusted_num_of_binop(bchild);
		modify_num_of_binop(bchild, result);
		if (bchild->right->type == NUM) {
			bchild->op = '+';
		}	
	} else if (bnode->op == '/') {
		modify_num_of_binop(bchild, bnum->value * get_num_of_binop(bchild));
	}

	// must detach bchild before free'ing
	if (bnode->left->type == BINARY_OP) {
		bnode->left = NULL;
	} else {
		bnode->right = NULL;
	}
	free_ast((node_t*)bnode);
	return bchild;
}

// assume each binop has 1 num
int64_t compute_num_for_binop_and_binop(binary_node_t *parent) {
	binary_node_t *left = (binary_node_t*)parent->left;
	binary_node_t *right = (binary_node_t*)parent->right;

	int64_t left_num = get_adjusted_num_of_binop(left);
	int64_t right_num = get_adjusted_num_of_binop(right) * (parent->op == '-' ? -1 : 1);

	return left_num + right_num;
}

// move into left bin_node
binary_node_t *merge_binop_and_binop(bnode_wrapper_t *w) {
	binary_node_t *bnode = (binary_node_t*)w->value;

	if (bnode->op == '+') {
		int64_t adjusted_sum = compute_num_for_binop_and_binop(bnode);
		modify_num_of_binop((binary_node_t*)bnode->left, adjusted_sum);
		if (((binary_node_t*)bnode->left)->right->type == NUM) {
			((binary_node_t*)bnode->left)->op = '+';
		}	

		binary_node_t *right = (binary_node_t*)bnode->right;
		if (right->left->type == NUM && right->op == '-') {
			bnode->op = '-';
		}

		// replace the right node
		bnode->right = (right->left->type == NUM ? right->right : right->left);

		// detach right->left OR right->right (whichever) from right
		if (right->left->type == NUM) {
			right->right = NULL;
		} else {
			right->left = NULL;
		}
		free_ast((node_t*)right);
	} else {
		binary_node_t *left = (binary_node_t*)bnode->left;
		binary_node_t *right = (binary_node_t*)bnode->right;

		// (X [] num) - (Y [] num) --> (X - Y) + aa_num
		if (left->right->type == NUM && right->right->type == NUM) {
			int64_t adjusted_sum = compute_num_for_binop_and_binop(bnode);
			replace_num_of_binop(left, get_non_num_of_binop(right));
			left->op = '-';
			bnode->op = '+';
			bnode->right = init_num_node(adjusted_sum);

			// destroy old num and the bin op (we create a new num node)
			if (right->left->type == NUM) {
				right->right = NULL;
			} else {
				right->left = NULL;
			}
			free_ast((node_t*)right);

		// (X [] num) - (num [] Y) --> (X + aa_num) - Y
		} else if (left->right->type == NUM && right->left->type == NUM) {
			int64_t adjusted_sum = compute_num_for_binop_and_binop(bnode);
			modify_num_of_binop(left, adjusted_sum);
			left->op = '+';
			bnode->op = (right->op == '+' ? '-' : '+');

			bnode->right = get_non_num_of_binop(right);
			if (right->left->type == NUM) {
				right->right = NULL;
			} else {
				right->left = NULL;
			}
			free_ast((node_t*)right);

		// (num [] X) - (Y [] num) --> (aa_num [] X) - Y
		} else if (left->left->type == NUM && right->right->type == NUM) {
			int64_t adjusted_sum = compute_num_for_binop_and_binop(bnode);
			modify_num_of_binop(left, adjusted_sum);
			bnode->op = '-';
			bnode->right = get_non_num_of_binop(right);
			if (right->left->type == NUM) {
				right->right = NULL;
			} else {
				right->left = NULL;
			}
			free_ast((node_t*)right);

		// (num [] X) - (num [] Y) --> (aa_num [] X) - Y
		} else {
			int64_t adjusted_sum = compute_num_for_binop_and_binop(bnode);
			modify_num_of_binop(left, adjusted_sum);
			bnode->op = (right->op == '+' ? '-' : '+');

			bnode->right = get_non_num_of_binop(right);
			if (right->left->type == NUM) {
				right->right = NULL;
			} else {
				right->left = NULL;
			}
			free_ast((node_t*)right);
		}
	}

	return bnode;
}

void inspect_binary_op(bnode_wrapper_t *w, void *unused) {
	(void)unused;
	binary_node_t *b = (binary_node_t*)w->value;
	if (b->left->type == NUM && b->right->type == NUM && b->op != '<' && b->op != '=' && b->op != '>') {
		replace_node(w, init_num_node(perform_op(((num_node_t*)b->left)->value,
												((num_node_t*)b->right)->value,
												b->op)));
		free_ast((node_t*)b);
	} else if (b->left->type != NUM && b->right->type == NUM && ((num_node_t*)b->right)->value == -1 && b->op == '/') {
		b->op = '*';
	} else if (((b->op == '+') && ((b->left->type == NUM && (((num_node_t*)b->left)->value == 0)) || (b->right->type == NUM && (((num_node_t*)b->right)->value == 0))))
				|| ((b->op == '-') && (b->right->type == NUM && (((num_node_t*)b->right)->value == 0)))
				|| ((b->op == '*') && ((b->left->type == NUM && (((num_node_t*)b->left)->value == 1)) || (b->right->type == NUM && (((num_node_t*)b->right)->value == 1))))
				|| ((b->op == '/') && (b->right->type == NUM && (((num_node_t*)b->right)->value == 1)))) {
		node_t *target = (b->left->type == NUM ? b->right : b->left);

		// detach target before free'ing b
		if (b->left->type == NUM) {
			b->right = NULL;
		} else {
			b->left = NULL;
		}
		free_ast((node_t*)b);

		replace_node(w, target);
	} else if (b->left->type == VAR && b->right->type == VAR && ((var_node_t*)b->left)->name == ((var_node_t*)b->right)->name
				&& b->op == '-') {
		replace_node(w, init_num_node(0));
		free_ast((node_t*)b);

	// [any order] (-1 * VAR) + var OR (var / -1) + var
	} else if ((b->left->type == VAR && b->right->type == BINARY_OP && binop_has_var((binary_node_t*)b->right) && binop_has_num((binary_node_t*)b->right))
				|| (b->right->type == VAR && b->left->type == BINARY_OP && binop_has_var((binary_node_t*)b->left) && binop_has_num((binary_node_t*)b->left))) {
		var_node_t *vnode1 = (var_node_t*)(b->left->type == VAR ? b->left : b->right);
		binary_node_t *bchild = (binary_node_t*)(b->left->type == VAR ? b->right : b->left);
		int64_t bchild_num = get_num_of_binop(bchild);
		var_node_t *vnode2 = (var_node_t*)get_non_num_of_binop(bchild);
		if (vnode1->name == vnode2->name && b->op == '+' && bchild_num == -1) {
			if (bchild->op == '*' || (bchild->op == '/' && bchild->right->type == NUM)) {
				replace_node(w, init_num_node(0));
				free_ast((node_t*)b);
			}
		}
	} else if (b->left->type == BINARY_OP && b->right->type == NUM && binop_has_num((binary_node_t*)b->left)) {
		binary_node_t *sub_b = (binary_node_t*)b->left;
		if (((b->op == '+' || b->op == '-') && (sub_b->op == '+' || sub_b->op == '-'))
			|| (b->op == '*' && sub_b->op == '*')
			|| (b->op == '/' && sub_b->op == '/' && sub_b->right->type == NUM)) {
			replace_node(w, (node_t*)merge_binop_and_num(w, RIGHT));
		}
	} else if (b->left->type == NUM && b->right->type == BINARY_OP && binop_has_num((binary_node_t*)b->right)) {
		binary_node_t *sub_b = (binary_node_t*)b->right;
		if (((b->op == '+') && (sub_b->op == '+' || sub_b->op == '-'))
			|| (b->op == '*' && sub_b->op == '*')) {
			replace_node(w, (node_t*)merge_binop_and_num(w, LEFT));
		}
	} else if (b->left->type == BINARY_OP && b->right->type == BINARY_OP) {
		if (binop_has_num((binary_node_t*)b->left) && binop_has_num((binary_node_t*)b->right)
			&& (((binary_node_t*)b->left)->op == '+' || ((binary_node_t*)b->left)->op == '-')
			&& (((binary_node_t*)b->right)->op == '+' || ((binary_node_t*)b->right)->op == '-')
			&& (b->op == '+' || b->op == '-')) {
			replace_node(w, (node_t*)merge_binop_and_binop(w));
		}
	}
}

node_t *condense_binary_op(binary_node_t *bnode) {
	queue_t *q = queue_new(free);
	queue_add_to_back(q, create_bnode_wrapper(NULL, LEFT, bnode));
	queue_traverse_front_to_back(q, (traversal_func_t)add_binary_op_children, q);
	// fprintf(stderr, "[*] Added all binary ops to queue. Size: %zu\n", queue_size(q));

	queue_traverse_back_to_front(q, (traversal_func_t)inspect_binary_op, NULL);

	// at this point, the first element contains our result
	node_t *result = ((bnode_wrapper_t*)queue_front(q))->value;

	// keep the wrappers in the queue; free them at the end
	queue_free(q);
	return result;
}

// to-do: PRINT(+(5, +(Y, +(3, X)))) --> 8 + X + Y ... involved multilevel 
void fold_constants(node_t *node) {
	switch (node->type) {
		case SEQUENCE: {
			sequence_node_t *snode = (sequence_node_t*)node;
			for (size_t i = 0; i < snode->statement_count; ++i) {
				fold_constants(snode->statements[i]);
			}
			break;
		}
		case PRINT: {
			print_node_t *pnode = (print_node_t*)node;
			if (pnode->expr->type == BINARY_OP) {
				pnode->expr = condense_binary_op((binary_node_t*)pnode->expr);
			}
			break;
		}
		case LET: {
			let_node_t *lnode = (let_node_t*)node;
			if (lnode->value->type == BINARY_OP) {
				lnode->value = condense_binary_op((binary_node_t*)lnode->value);
			}
			break;
		}
		case IF: {
			if_node_t *inode = (if_node_t*)node;
			inode->condition = (binary_node_t*)condense_binary_op(inode->condition);
			fold_constants(inode->if_branch);
			if (inode->else_branch) {
				fold_constants(inode->else_branch);
			}
			break;
		}
		case WHILE: {
			while_node_t *wnode = (while_node_t*)node;
			wnode->condition = (binary_node_t*)condense_binary_op(wnode->condition);
			fold_constants(wnode->body);
			break;
		}
		default: exit(3); // should not happen
	}
}


/**
 * PREDICTION (constant expression) ALGORITHM (somewhat aggressive...)
 * 
 * Iterate through statements:
 * 		LET: IF IT HAS not been tracked yet: track it & evaluate its initial value
 * 			if, when evaluating it, we find that it references a tainted variable, taint the current variable
 * 		IF: Evaluate the condition: if we CANNOT (b/c of tainting reasons), do nothing
 * 			if we CAN evaluate it, then determine which statements are executed (if any) and simply replace them w/ the if
 * 		WHILE: 
 * 			1) Upon encountering a while; clone the AST
 * 			2) Traverse through the while statement:
 * 			3) 		We evaluate if statements [note, here we are going through the blocks; NOT replacing / making new nodes]
 * 			4) 		If we end up tainting a variable, we update the tainted map and exit/ returning to step 2
 * 			5) At this point, we have completed a full traversal of the while loop
 * 			6) Delete the clone & traverse the original, this time w/ an updated tainted map
 * 			The reason we use a clone instead of the actual AST is b/c we call evaluate_expr which deletes nodes/etc
 * 				(easier to just use evaluate_expr)
 * 
 * 			Why do we iterate several times instead of just once?
 * 				Consider:
 * 					LET R = 100
 * 					LET X = 2
 * 					WHILE X < 1000
 * 
 * 						IF R = X
 * 							LET R = R + X
 * 						END IF
 * 
 * 						X = X + 1
 * 
 * 					END WHILE
 * 			If we only do 1 pass, because we evaluate if statements inside while discovering, R will NOT be tainted
 * 			because R and X are not tainted, so it will evaluate R = X --> false, so the inside if is not evaluated.
 * 			However, later, it finds X is tainted, but that does not propagate to the R = X part, and we find that
 * 			R is not tainted when it should be.
 * 
 *		PRINT: If we can evaluate, replace binop/var w/ the constant
 * 
 * 		At the end: remove any let statements for which we cannot find any references (to prints)
 * 						requires maintaing a variable connection list --> 
 * 					remove empty while/if blocks
 */

typedef struct {
	bool tainted;
	bool in_scope;	// technically not needed... if we assume all variables declared appropriately
	int64_t value;
} var_data_t;

node_t *evaluate_expr(node_t *expr, var_data_t *var_data) {
	switch (expr->type) {
		case NUM: return expr;
		case VAR: {
			var_node_t *v = (var_node_t*)expr;
			var_data_t vdata = var_data[v->name - 'A'];
			fprintf(stderr, "evaluating %c: ", v->name);
			if (vdata.tainted || !vdata.in_scope) {
				fprintf(stderr, "tainted!\n");
				return expr;
			} else {
				fprintf(stderr, "not tainted!\n");
				free_ast(expr);
				return init_num_node(vdata.value);
			}
		}
		case BINARY_OP: {
			binary_node_t *b = (binary_node_t*)expr;
			b->left = evaluate_expr((node_t*)b->left, var_data);
			b->right = evaluate_expr((node_t*)b->right, var_data);
			if (b->left->type == NUM && b->right->type == NUM && b->op != '=' && b->op != '<' && b->op != '>') {
				int64_t new_val = perform_op(((num_node_t*)b->left)->value, ((num_node_t*)b->right)->value, b->op);
				free_ast(expr);
				return init_num_node(new_val);
			} else {
				return (node_t*)b;
			}
		}
		default: exit(4);
	}
}

// shifts all statements down & adjusts statement_count
void condense_ast_sequence(sequence_node_t *snode) {
	size_t current_size = 0;
	for (size_t current_idx = 0; current_idx < snode->statement_count; ++current_idx) {
		if (snode->statements[current_idx]) {
			snode->statements[current_size++] = snode->statements[current_idx];
		}
	}
	snode->statement_count = current_size;
}

bool discover(node_t *node, var_data_t *var_data) {
	switch (node->type) {
		case SEQUENCE: {
			sequence_node_t *snode = (sequence_node_t*)node;
			for (size_t i = 0; i < snode->statement_count; ++i) {
				if (discover(snode->statements[i], var_data)) return true;
			}
			break;
		}
		case LET: {
			let_node_t *lnode = (let_node_t*)node;
			if (var_data[lnode->var - 'A'].tainted) break;

			fprintf(stderr, "tainting in discover '%c'\n", lnode->var);
			var_data[lnode->var - 'A'].tainted = true;
			// in general, it's tricky to determine if a variable has truly been tainted inside a while loop
			// for now, we just taint all variables that are set to some (which can definitely lead to false positives) value
			return true;
		}
		case IF: {
			if_node_t *inode = (if_node_t*)node;
			inode->condition = (binary_node_t*)evaluate_expr((node_t*)inode->condition, var_data);
			if (inode->condition->left->type == NUM && inode->condition->right->type == NUM) {
				bool result = perform_comp(((num_node_t*)inode->condition->left)->value, ((num_node_t*)inode->condition->right)->value, inode->condition->op);
				if (result) {
					if (discover(inode->if_branch, var_data)) return true;
				} else if (inode->else_branch) {
					if (discover(inode->else_branch, var_data)) return true;
				}
			} else {
				if (discover(inode->if_branch, var_data)) return true;
				if (inode->else_branch && discover(inode->else_branch, var_data)) return true;
			}
			break;	
		}
		case WHILE: {
			while_node_t *w = (while_node_t*)node;
			if (discover(w->body, var_data)) return true;
			break;
		}
		default: break;
	}
	return false;
}

// return NULL if statement should be deleted
node_t *predict(node_t *ast, var_data_t *var_data) {
	switch (ast->type) {
		case SEQUENCE: {
			fprintf(stderr, "seq\n");
			sequence_node_t *snode = (sequence_node_t*)ast;
			bool did_delete = false;
			for (size_t i = 0; i < snode->statement_count; ++i) {
				node_t *result = predict(snode->statements[i], var_data);
				if (result) {
					fprintf(stderr, "yes, theres a result!\n");
					if (result != snode->statements[i]) {
						fprintf(stderr, "must free!\n");
						free_ast(snode->statements[i]);
						snode->statements[i] = result;
					}
				} else {
					free_ast(snode->statements[i]);
					snode->statements[i] = NULL;
					did_delete = true;
				}
			}
			if (did_delete) condense_ast_sequence(snode);
			break;
		}
		case LET: {
			fprintf(stderr, "let\n");
			let_node_t *lnode = (let_node_t*)ast;
			var_data_t *vdata = &var_data[lnode->var - 'A'];
			if (vdata->tainted) break;

			lnode->value = evaluate_expr(lnode->value, var_data);

			if (lnode->value->type != NUM) {
				vdata->tainted = true;
				fprintf(stderr, "tainting '%c'\n", lnode->var);
			} else {
				int64_t new_val = ((num_node_t*)lnode->value)->value;
				if (vdata->in_scope &&  vdata->value == new_val) {
					return NULL;
				} else {
					vdata->value = new_val;
				}
			}
			vdata->in_scope = true;
			break;
		}
		case PRINT: {
			fprintf(stderr, "print\n");
			print_node_t *pnode = (print_node_t*)ast;
			pnode->expr = evaluate_expr(pnode->expr, var_data);
			break;
		}
		case IF: {
			fprintf(stderr, "if\n");
			if_node_t *inode = (if_node_t*)ast;
			inode->condition = (binary_node_t*)evaluate_expr((node_t*)inode->condition, var_data);
			if (inode->condition->left->type == NUM && inode->condition->right->type == NUM) {
				bool result = perform_comp(((num_node_t*)inode->condition->left)->value, ((num_node_t*)inode->condition->right)->value, inode->condition->op);
				if (result) {
					fprintf(stderr, "ifsttt\n");
					// replace w/ if branch
					node_t *result = inode->if_branch;
					node_t *sub_result = predict(result, var_data);
					if (inode->if_branch == sub_result) {
						inode->if_branch = NULL;
					}

					// recurse
					return sub_result;
				} else if (inode->else_branch) {
					fprintf(stderr, "here??\n");
					node_t *result = inode->else_branch;
					node_t *sub_result = predict(result, var_data);
					if (inode->else_branch == sub_result) {
						inode->else_branch = NULL;
					}

					// recurse
					return sub_result;
				} else {
					fprintf(stderr, "no!\n");
					return NULL;
				}
			}	
		}
		// search and taint
		case WHILE: {
			fprintf(stderr, "while\n");
			while_node_t *w = (while_node_t*)ast;

			node_t *w_clone = copy_ast(ast);
			while (discover(w_clone, var_data)) {
				fprintf(stderr, "discovering again!\n");
				free_ast(w_clone);
				w_clone = copy_ast(ast);
			}
			free_ast(w_clone);
			
			// can delete while if condition never satisfied
			w->condition = (binary_node_t*)evaluate_expr((node_t*)w->condition, var_data);
			if (w->condition->left->type == NUM && w->condition->right->type == NUM) {
				bool result = perform_comp(((num_node_t*)w->condition->left)->value, ((num_node_t*)w->condition->right)->value, w->condition->op);
				if (!result) {
					return NULL;
				}
			}
			
			node_t *old_body = w->body;
			w->body = predict(w->body, var_data);
			if (!w->body) {
				// don't get rid of it... incase infinite while loop is intended...
				free_ast(old_body);
				w->body = init_sequence_node(0, NULL);
			} else if (w->body != old_body) {
				free_ast(old_body);
			}
 			break;
		}
		default: exit(5);
	}
	return ast;
}

node_t *strip_unnecessary_let_statements(node_t *ast, size_t *refs) {
	switch (ast->type) {
		case SEQUENCE: {
			sequence_node_t *snode = (sequence_node_t*)ast;
			bool did_delete = false;

			for (size_t i = 0; i < snode->statement_count; ++i) {
				node_t *result = strip_unnecessary_let_statements(snode->statements[i], refs);
				if (!result) {
					free_ast(snode->statements[i]);
					snode->statements[i] = NULL;
					did_delete = true;
				}
			}
			if (did_delete) condense_ast_sequence(snode);
			break;
		}
		case LET: {
			let_node_t *lnode = (let_node_t*)ast;
			if (refs[lnode->var - 'A'] == 0) {
				return NULL;
			}
			break;
		}
		case IF: {
			if_node_t *inode = (if_node_t*)ast;
			inode->if_branch = strip_unnecessary_let_statements(inode->if_branch, refs);
			if (inode->else_branch) {
				inode->else_branch = strip_unnecessary_let_statements(inode->else_branch, refs);
				if (inode->if_branch == NULL && inode->else_branch == NULL) {
					return NULL;
				} else if (inode->if_branch == NULL) {
					// since we can't leave it null
					inode->if_branch = init_sequence_node(0, NULL);
				}
			} else if (inode->if_branch == NULL) {
				return NULL;
			}
			break;
		}
		case WHILE: {
			while_node_t *wnode = (while_node_t*)ast;
			wnode->body = strip_unnecessary_let_statements(wnode->body, refs);
			if (wnode->body == NULL) {
				// don't get rid of it... incase while loop is intended...
				wnode->body = init_sequence_node(0, NULL);
			}
			break;
		}
		default: break;
	}
	return ast;
}

void count_refs(node_t *ast, size_t *refs, char ignore_variable) {
	switch (ast->type) {
		case BINARY_OP: {
			count_refs(((binary_node_t*)ast)->left, refs, ignore_variable);
			count_refs(((binary_node_t*)ast)->right, refs, ignore_variable);
			break;
		}
		case NUM: break;
		case VAR: {
			char c = ((var_node_t*)ast)->name;
			if (c != ignore_variable) {
				refs[c - 'A']++;
			}
			break;
		}
		case SEQUENCE: {
			sequence_node_t *snode = (sequence_node_t*)ast;
			for (size_t i = 0; i < snode->statement_count; ++i) {
				count_refs(snode->statements[i], refs, ' ');
			}
			break;
		}
		case PRINT: {
			count_refs(((print_node_t*)ast)->expr, refs, ' ');
			break;
		}
		case LET: {
			let_node_t *lnode = (let_node_t*)ast;
			count_refs(lnode->value, refs, lnode->var);
			break;
		}
		case IF: {
			if_node_t *inode = (if_node_t*)ast;
			count_refs((node_t*)inode->condition, refs, ' ');
			count_refs(inode->if_branch, refs, ' ');
			if (inode->else_branch) count_refs(inode->else_branch, refs, ' ');
			break;
		}
		case WHILE: {
			while_node_t *wnode = (while_node_t*)ast;
			count_refs((node_t*)wnode->condition, refs, ' ');
			count_refs(wnode->body, refs, ' ');
			break;
		}
	}
}

node_t *optimize_ast(node_t *ast) {
	fold_constants(ast);
	var_data_t var_data[26];
	memset(var_data, 0, 26 * sizeof(var_data_t));
	ast = predict(ast, var_data);
	if (ast) {
		// determine references for all variables
		size_t refs[26];
		memset(refs, 0, 26 * sizeof(size_t));
		count_refs(ast, refs, ' ');
		return strip_unnecessary_let_statements(ast, refs);
	} else {
		return NULL;
	}
}

/**
 * To-do
 * 1) More intelligent constant folding: PRINT (X + 5) + (Y + 10) + (Z - 10) --> is chained tactic
 * 2) Remove unnecesary push/pop (post processor)
 * 		LET Z = I - (5 - (2 - (B * A)))
 *			Our algorithm would reserve %rdi for I, and then 5 (after which point it must push), without actually using them yet
 *			resulting in unnecessary push/pops --> very niche case, but our post processor should get rid of this
 * 3) [done] Remove pre/post push/pops (post processor)
 * 4) [done] Optimize while/if condition checking
 * 5) [done] Very small optimization w/ division and let statements
 * 
 * benchmarks:
 * opt1: ~206
 * opt2: ~608
 * 
 */