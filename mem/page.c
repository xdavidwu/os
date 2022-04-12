#include "errno.h"
#include "exceptions.h"
#include "kio.h"
#include "page.h"
#include "stdlib.h"
#include <stdint.h>

static void *const page_base = (void *)0x10000000;

#define PAGE_UNIT	(4 * 1024)

struct list_entry {
	int idx;
	struct list_entry *next, *prev;
};

#define PAGE_BUDDIES_SZ	(0x10000000 / PAGE_UNIT)
#define BUDDY_IS_BUDDY	0x7f

static struct {
	uint8_t status;
	struct list_entry *ent;
} page_buddies[PAGE_BUDDIES_SZ];

#define MAX_ORD	16
static struct list_entry free_lists[MAX_ORD + 1] = {0};

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

void *page_alloc(int ord) {
	for (int a = ord; a <= MAX_ORD; a++) {
		if (free_lists[a].next) {
			DISABLE_INTERRUPTS();
			struct list_entry *taken = free_lists[a].next;
			int take = taken->idx;
			free_lists[a].next = taken->next;
			kputs("take page: ");
			kput16x(take);
			kputs(" ord: ");
			kput16x(a);
			kputs("\n");
			while (a > ord) {
				a--;
				struct list_entry *ptrb = free_lists[a].next;
				free_lists[a].next = malloc(sizeof(struct list_entry));
				free_lists[a].next->idx = take + (1 << a);
				ptrb->prev = free_lists[a].next;
				free_lists[a].next->next = ptrb;
				free_lists[a].next->prev = &free_lists[a];
				page_buddies[free_lists[a].next->idx].status = a;
				page_buddies[free_lists[a].next->idx].ent = free_lists[a].next;
				kputs(" release page: ");
				kput16x(free_lists[a].next->idx);
				kputs(" ord: ");
				kput16x(a);
				kputs("\n");
			}
			page_buddies[take].status = a;
			page_buddies[take].ent = NULL;
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
	kputs("free page: ");
	kput16x(idx);
	kputs(" ord: ");
	kput16x(ord);
	kputs("\n");
	for (ord += 1; ord <= MAX_ORD; ord++) {
		int bud = idx ^ (1 << ord);
		if (!page_buddies[bud].ent) {
			break;
		}
		page_buddies[bud].ent->next->prev = page_buddies[bud].ent->prev;
		page_buddies[bud].ent->prev->next = page_buddies[bud].ent->next;
		//TODO free
		page_buddies[bud].ent = NULL;
		page_buddies[bud].status = BUDDY_IS_BUDDY;
		idx = bud < idx ? bud : idx;
		kputs(" merge page: ");
		kput16x(bud);
		kputs(" ord: ");
		kput16x(ord);
		kputs("\n");
	}
	ord -= 1;
	struct list_entry *ptrb = free_lists[ord].next;
	free_lists[ord].next = malloc(sizeof(struct list_entry));
	free_lists[ord].next->idx = idx;
	ptrb->prev = free_lists[ord].next;
	free_lists[ord].next->next = ptrb;
	free_lists[ord].next->prev = &free_lists[ord];
	page_buddies[idx].status = ord;
	page_buddies[idx].ent = free_lists[ord].next;
	ENABLE_INTERRUPTS();
}

void page_alloc_init() {
	free_lists[MAX_ORD].next = malloc(sizeof(struct list_entry));
	free_lists[MAX_ORD].next->idx = 0;
	free_lists[MAX_ORD].next->next = NULL;
	free_lists[MAX_ORD].next->prev = &free_lists[MAX_ORD];
	for (int a = 1; a < PAGE_BUDDIES_SZ; a++) {
		page_buddies[a].status = BUDDY_IS_BUDDY;
	}
	page_buddies[0].status = MAX_ORD;
	page_buddies[0].ent = free_lists[MAX_ORD].next;
}
