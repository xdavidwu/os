#include "errno.h"
#include "exceptions.h"
#include "kio.h"
#include "page.h"
#include "stdlib.h"
#include <stdint.h>

static void *const page_base = (void *)0x10000000;

#define PAGE_BUDDIES_SZ	(0x10000000 / PAGE_UNIT)
#define BUDDY_IS_BUDDY	0x7f

static struct page_buddy {
	uint8_t status;
	bool reserved;
	struct page_buddy *next, *prev;
} page_buddies[PAGE_BUDDIES_SZ];

#define MAX_ORD	14
static struct page_buddy free_lists[MAX_ORD + 1];

void *prepage_malloc(size_t size);

static void kput64x(uint64_t val) {
	char hexbuf[17];
	hexbuf[16] = '\0';
	for (int i = 0; i < 16; i++) {
		int dig = val & 0xf;
		hexbuf[15 - i] = (dig > 9 ? 'a' - 10 : '0') + dig;
		val >>= 4;
	}
	kputs(hexbuf);
}

static void kput16x(uint16_t val) {
	char hexbuf[5];
	hexbuf[4] = '\0';
	for (int i = 0; i < 4; i++) {
		int dig = val & 0xf;
		hexbuf[3 - i] = (dig > 9 ? 'a' - 10 : '0') + dig;
		val >>= 4;
	}
	kputs(hexbuf);
}

void print_status() {
	for (int a = 0; a <= MAX_ORD; a++) {
		kputs("ord: ");
		kput16x(a);
		if (free_lists[a].next) {
			int c = 1;
			struct page_buddy *ptr = free_lists[a].next;
			kput16x(ptr - page_buddies);
			while (ptr->next) {
				ptr = ptr->next;
				c++;
			}
			kputs(" avail ");
			kput16x(c);
			kputs("\n");
		} else {
			kputs(" noavail\n");
		}
	}
}
void *page_alloc(int ord) {
	for (int a = ord; a <= MAX_ORD; a++) {
		if (free_lists[a].next) {
			DISABLE_INTERRUPTS();
			struct page_buddy *const taken = free_lists[a].next;
			const int take = taken - page_buddies;
			free_lists[a].next = taken->next;
			if (taken->next) {
				taken->next->prev = &free_lists[a];
			}
			page_buddies[take].status = ord;
			page_buddies[take].prev = NULL;
			kputs("take page: ");
			kput16x(take);
			kputs(" ord: ");
			kput16x(a);
			kputs("\n");
			while (a > ord) {
				a--;
				struct page_buddy *const ptrb = free_lists[a].next;
				const int idx = take + (1 << a);
				free_lists[a].next = &page_buddies[idx];
				if (ptrb) {
					ptrb->prev = free_lists[a].next;
				}
				free_lists[a].next->next = ptrb;
				free_lists[a].next->prev = &free_lists[a];
				page_buddies[idx].status = a;
				kputs(" release page: ");
				kput16x(idx);
				kputs(" ord: ");
				kput16x(a);
				kputs("\n");
			}
			ENABLE_INTERRUPTS();
			return page_base + take * PAGE_UNIT;
		}
	}
	return (void *)-ENOMEM;
}

void page_free(void *page) {
	int idx = (page - page_base) / PAGE_UNIT;
	DISABLE_INTERRUPTS();
	int ord = page_buddies[idx].status;
	page_buddies[idx].status = BUDDY_IS_BUDDY;
	page_buddies[idx].prev = NULL;
	kputs("free page: ");
	kput16x(idx);
	kputs(" ord: ");
	kput16x(ord);
	kputs("\n");
	while (ord < MAX_ORD) {
		int bud = idx ^ (1 << ord);
		if (!page_buddies[bud].prev) {
			break;
		}
		if (page_buddies[bud].next) {
			page_buddies[bud].next->prev = page_buddies[bud].prev;
		}
		page_buddies[bud].prev->next = page_buddies[bud].next;
		page_buddies[bud].status = BUDDY_IS_BUDDY;
		page_buddies[bud].prev = NULL;
		idx = bud < idx ? bud : idx;
		kputs(" merge page: ");
		kput16x(bud);
		kputs(" ord: ");
		kput16x(ord);
		kputs("\n");
		ord++;
	}
	struct page_buddy *ptrb = free_lists[ord].next;
	free_lists[ord].next = &page_buddies[idx];
	if (ptrb) {
		ptrb->prev = free_lists[ord].next;
	}
	free_lists[ord].next->next = ptrb;
	free_lists[ord].next->prev = &free_lists[ord];
	page_buddies[idx].status = ord;
	kputs(" create: ");
	kput16x(idx);
	kputs(" ord: ");
	kput16x(ord);
	kputs("\n");
	ENABLE_INTERRUPTS();
}

void page_alloc_preinit() {
	for (int a = 0; a < PAGE_BUDDIES_SZ; a++) {
		page_buddies[a].status = BUDDY_IS_BUDDY;
	}
}

void page_alloc_init() {
	DISABLE_INTERRUPTS();
	for (int a = 0; a <= MAX_ORD; a++) {
		free_lists[a].next = NULL;
		free_lists[a].prev = NULL;
	}
	int current_index = 0, step = MAX_ORD, section_end = current_index;
	while (section_end != PAGE_BUDDIES_SZ) {
		while (!page_buddies[section_end].reserved && section_end < PAGE_BUDDIES_SZ) {
			section_end++;
		}
		while (current_index < section_end) {
			while ((section_end - current_index) < (1 << step)) {
				step--;
			}
			struct page_buddy *ptrb = free_lists[step].next;
			free_lists[step].next = &page_buddies[current_index];
			if (ptrb) {
				ptrb->prev = free_lists[step].next;
			}
			free_lists[step].next->next = ptrb;
			free_lists[step].next->prev = &free_lists[step];
			page_buddies[current_index].status = step;
			current_index += (1 << step);
		}
		while (page_buddies[current_index].reserved && current_index < PAGE_BUDDIES_SZ) {
			current_index++;
		}
		section_end = current_index;
	}
	ENABLE_INTERRUPTS();
}

void mem_reserve(void *start, void *end) {
	int end_idx = ((end - 1) - page_base) / PAGE_UNIT;
	int start_idx = (start - page_base) / PAGE_UNIT;
	for (int a = start_idx; a <= end_idx; a++) {
		page_buddies[a].reserved = true;
	}
}
