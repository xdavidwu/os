#include "aarch64/vmem.h"
#include "exceptions.h"
#include "page.h"
#include "vmem.h"
#include <stdint.h>

static void pagetable_init(uint64_t *pa) {
	for (int i = 0; i < 512; i++) {
		pa[i] = 0;
	}
}

uint64_t *pagetable_new() {
	uint64_t *res = page_alloc(1);
	pagetable_init((uint64_t *)((uint64_t)res + HIGH_MEM_OFFSET));
	return res;
}

void pagetable_insert(uint64_t *pagetable, int permission, void *dst, void *src) {
	uint64_t flags = 0;
	if ((permission & PAGETABLE_USER_W) != PAGETABLE_USER_W) {
		flags |= PD_RO;
	}
	if ((permission & PAGETABLE_USER_X) != PAGETABLE_USER_X) {
		flags |= PD_USER_NX;
	}
	uint64_t *pgd = (uint64_t *)((uint64_t)pagetable + HIGH_MEM_OFFSET);
	int index = ADDR_PGD_IDX((uint64_t)(src));
	if ((pgd[index] & PD_TYPE_MASK) != PD_TABLE) {
		pgd[index] = (uint64_t)pagetable_new() | PD_TABLE;
	}
	uint64_t *pud = (uint64_t *)((pgd[index] & PD_ADDR_MASK) + HIGH_MEM_OFFSET);
	index = ADDR_PUD_IDX((uint64_t)(src));
	if ((pud[index] & PD_TYPE_MASK) != PD_TABLE) {
		pud[index] = (uint64_t)pagetable_new() | PD_TABLE;
	}
	uint64_t *pmd = (uint64_t *)((pud[index] & PD_ADDR_MASK) + HIGH_MEM_OFFSET);
	index = ADDR_PMD_IDX((uint64_t)(src));
	if ((pmd[index] & PD_TYPE_MASK) != PD_TABLE) {
		pmd[index] = (uint64_t)pagetable_new() | PD_TABLE;
	}
	uint64_t *pte = (uint64_t *)((pmd[index] & PD_ADDR_MASK) + HIGH_MEM_OFFSET);
	index = ADDR_PTE_IDX((uint64_t)(src));
	pte[index] = (uint64_t)dst | PD_ACCESS | PD_USER | (MAIR_IDX_NORMAL_NOCACHE << 2) | PD_TABLE | flags;
}

void pagetable_insert_range(uint64_t *pagetable, int permission, void *dst, void *src, int length) {
	for (int a = 0; a < length; a++) {
		pagetable_insert(pagetable, permission, dst, src);
		dst += PAGE_UNIT;
		src += PAGE_UNIT;
	}
}

static void pagetable_destroy_layer(uint64_t *pagetable, int layer) {
	layer--;
	uint64_t *pa = (uint64_t *)((uint64_t)pagetable + HIGH_MEM_OFFSET);
	if (layer) {
		for (int i = 511; i >= 0; i--) {
			if ((pa[i] & PD_TYPE_MASK) == PD_TABLE) {
				pagetable_destroy_layer((uint64_t *)(pa[i] & PD_ADDR_MASK), layer);
			} else if ((pa[i] & PD_TYPE_MASK) == PD_BLOCK && !(pa[i] & PAGE_STICKY)) {
				page_free((void *)(pa[i] & PD_ADDR_MASK));
			}

		}
	} else {
		for (int i = 511; i >= 0; i--) {
			if ((pa[i] & PD_TYPE_MASK) == PD_TABLE && !(pa[i] & PAGE_STICKY)) {
				page_free((void *)(pa[i] & PD_ADDR_MASK));
			}
		}
	}
	page_free(pagetable);
}

void pagetable_destroy(uint64_t *pagetable) {
	pagetable_destroy_layer(pagetable, 4);
}

void *pagetable_translate(uint64_t *pagetable, void *addr) {
	uint64_t addru = (uint64_t)addr;
	uint64_t res;

	DISABLE_INTERRUPTS();
	__asm__ ("dsb ish\nmsr ttbr0_el1, %0\ntlbi vmalle1is\ndsb ish\nisb" : : "r" (pagetable));
	__asm__ ("at s1e0r, %0" : : "r" (addru));
	__asm__ ("mrs %0, par_el1" : "=r" (res));
	ENABLE_INTERRUPTS();
	return (void *)((res & PD_ADDR_MASK) | ((uint64_t)addr & ((1 << 12) - 1)));
}
