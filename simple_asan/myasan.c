#define _GNU_SOURCE 1
#define _POSIX_C_SOURCE 199309L

#include <inttypes.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "util.h"

static const size_t PAGE_SIZE = 4096;
typedef uint8_t page_t[PAGE_SIZE];

static void *const START_PAGE = (void *) ((size_t) 1 << 32);
static const size_t MAX_HEAP_SIZE = (size_t) 1 << 30;
static const int HEAP_MMAP_FLAGS = MAP_ANONYMOUS | MAP_PRIVATE;
static const size_t HEADER_MAGIC = 0x0123456789ABCDEF;

typedef struct {
	size_t magic;
	size_t size;
	bool is_allocated;
	void *payload_addr;
} header_t;

static bool is_initialized = false;
static page_t *current_page;
static page_t *changed_page;

//
//	PAGE UTILITIES
//

static inline bool is_addr_in_heap(void *addr) {
	return addr >= START_PAGE && addr <= (void*)current_page; // <= seems to be behavior wanted by test cases
}

static inline size_t pages_round_up(size_t size) {
	return (size + PAGE_SIZE - 1) / PAGE_SIZE;
}

static inline void *get_curr_page(void *addr) {
	return addr - ((size_t)addr % PAGE_SIZE);
}

static inline void *get_prev_page(void *addr) {
	return get_curr_page(addr) - PAGE_SIZE;
}

static void set_header(page_t *header_page, size_t size, bool is_allocated, void *payload_addr) {
	header_t *header = (header_t*)header_page;
	header->magic = HEADER_MAGIC;
	header->size = size;
	header->is_allocated = is_allocated;
	header->payload_addr = payload_addr;
}

//
//	LEAK CHECK
//

static void release_resources(void) {
	fclose(stdout);
	mprotect(START_PAGE, MAX_HEAP_SIZE, PROT_READ | PROT_WRITE);
}

static void check_for_leaks(void) {
	// Prevent memory leaks from stdout
	release_resources();
	for (page_t *page = START_PAGE; page != current_page; ++page) {
		header_t *header = (header_t*)page;
		if (header->magic == HEADER_MAGIC && header->is_allocated) {
			report_memory_leak(header->payload_addr, header->size);
		}
	}
}

//
//	HEAP ACCESS CHECK
//

// SIGSEGV occurs when:
// (1) Addr is in header page (invalid)
// (3) Addr is in a full page of a block that has been freed (invalid)

static void sigsegv_handler(int sig, siginfo_t *info, void *context) {
	ucontext_t *ucontext = (ucontext_t*)context;
	void *addr = info->si_addr;

	release_resources();
	if (!is_addr_in_heap(addr)) {
		report_seg_fault(addr);
	} else {
		report_invalid_heap_access(addr);
	}
}

//
//	ASAN INIT
//

static void asan_init(void) {
	if (is_initialized) {
		return;
	}

	struct sigaction sigsegv_act = {
		.sa_sigaction = sigsegv_handler,
		.sa_flags = SA_SIGINFO
	};
	sigaction(SIGSEGV, &sigsegv_act, NULL);

	current_page = mmap(START_PAGE, MAX_HEAP_SIZE,
						PROT_NONE,
						HEAP_MMAP_FLAGS, -1, 0);
	assert(current_page == START_PAGE);

	atexit(check_for_leaks);

	is_initialized = true;
}


//
//	MALLOC
//

static inline void *compute_payload_addr(page_t *header_page, size_t size) {
	size_t remainder = size % PAGE_SIZE;
	if (remainder == 0) {
		return header_page + 1;
	} else {
		void *page_one_end = header_page + 2;
		return page_one_end - remainder;
	}
}

void *malloc(size_t size) {
	asan_init();

	size_t pages_necessary = pages_round_up(size);
	page_t *header_page = current_page;

	mprotect(header_page, PAGE_SIZE, PROT_READ | PROT_WRITE);
	set_header(header_page, size, true, compute_payload_addr(header_page, size));
	void *payload_addr = ((header_t*)header_page)->payload_addr;

	// set protection (recall that everything by default is PROT_NONE)
	for (size_t i = 1; i <= pages_necessary; ++i) {
		mprotect(header_page + i, PAGE_SIZE, PROT_READ | PROT_WRITE);
	}
	mprotect(header_page, PAGE_SIZE, PROT_NONE);

	current_page += 1 + pages_necessary;
	return payload_addr;
}

//
//	FREE
//

void free(void *ptr) {
	asan_init();
	if (ptr == NULL) return;

	// unlock the previous page (we assume it is header, which is locked)
	// if it is not, we error anyways (so don't need to remember old protection settings)
	page_t *header_page = (page_t*)get_prev_page(ptr);
	header_t *header = (header_t*)get_prev_page(ptr);
	if (!is_addr_in_heap(header)) {
		release_resources();
		report_invalid_free(ptr);
	} else {
		mprotect(header_page, PAGE_SIZE, PROT_READ | PROT_WRITE);

		if (header->magic != HEADER_MAGIC || header->payload_addr != ptr) {
			release_resources();
			report_invalid_free(ptr);
		} else if (!header->is_allocated) {
			release_resources();
			report_double_free(ptr, header->size);
		} else {
			header->is_allocated = false;
			// now, PROT_NONE all pages (including header again)
			size_t page_count = pages_round_up(header->size);
			for (size_t i = 0; i <= page_count; ++i) {
				mprotect(header_page + i, PAGE_SIZE, PROT_NONE);
			}
		}
	}
}