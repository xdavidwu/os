#ifndef VMEM_H
#define VMEM_H

#include <stdint.h>

enum {
	PAGETABLE_USER_W = 1 << 0,
	PAGETABLE_USER_X = 1 << 1,
};

uint64_t *pagetable_new();
void pagetable_insert(uint64_t *pagetable, int permission, void *dst, void *src);
void pagetable_insert_range(uint64_t *pagetable, int permission, void *dst, void *src, int length);
void pagetable_ondemand(uint64_t *pagetable, int permission, void *src);
void pagetable_ondemand_range(uint64_t *pagetable, int permission, void *src, int length);
void pagetable_demand(uint64_t *pagetable, void *dst, void *src);
void pagetable_destroy(uint64_t *pagetable);
void *pagetable_translate(uint64_t *pagetable, void *addr);

#endif
