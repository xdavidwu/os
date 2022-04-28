#include "exceptions.h"
#include "page.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static int align = 128 / 8;

struct pages {
	void *addr;
	size_t count;
};

struct bookkeeping_page_content {
	size_t sz;
	struct pages pages[];
};

struct mem_header {
	union {
		struct {
			bool sliced;
			bool availiable;
			size_t sz;
		};
		uint64_t _sz[2];
	};
};

static struct bookkeeping_page_content *bookkeeping_page;

void malloc_init() {
	bookkeeping_page = page_alloc(1);
	bookkeeping_page->sz = 0;
}

void *malloc(size_t size) {
	size = (size + align - 1) / align * align;
	DISABLE_INTERRUPTS();
	for (int a = 0; a < bookkeeping_page->sz; a++) {
		struct mem_header *mem = bookkeeping_page->pages[a].addr;
		void *end = (void *)mem + PAGE_UNIT * (bookkeeping_page->pages[a].count);
		while ((void *)(mem + 1) + size <= end) {
			if (!mem->sliced) {
				mem->sliced = true;
				mem->availiable = false;
				mem->sz = size;
				void *ptr = mem + 1;
				struct mem_header *next = ptr + size;
				if ((void *)(next + 1) < end) {
					next->sliced = false;
				}
				ENABLE_INTERRUPTS();
				return ptr;
			}
			if (mem->availiable && mem->sz >= size) {
				mem->availiable = false;
				ENABLE_INTERRUPTS();
				return mem + 1;
			}
			mem = (void *)(mem + 1) + mem->sz;
		}
	}
	size_t min_sz = sizeof(struct mem_header) + size;
	int page_count = (min_sz + PAGE_UNIT - 1) / PAGE_UNIT;
	int ord = 0;
	while (page_count > (1 << ord)) {
		ord++;
	}
	struct mem_header *mem = page_alloc(ord);
	bookkeeping_page->pages[bookkeeping_page->sz].addr = mem;
	bookkeeping_page->pages[bookkeeping_page->sz].count = 1 << ord;
	bookkeeping_page->sz++;
	mem->sliced = true;
	mem->availiable = false;
	mem->sz = size;
	void *ptr = mem + 1;
	struct mem_header *next = ptr + size;
	if ((void *)(next + 1) < (void *)mem + PAGE_UNIT * (1 << ord)) {
		next->sliced = false;
	}
	ENABLE_INTERRUPTS();
	return ptr;
}

void free(void *ptr) {
	if (!ptr) {
		return;
	}
	struct mem_header *mem = ptr - sizeof(struct mem_header);
	mem->availiable = true;
}
