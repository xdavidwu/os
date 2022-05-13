#ifndef VMEM_H
#define VMEM_H

#include <stdint.h>

uint64_t *pagetable_new();
void pagetable_insert(uint64_t *pagetable, void *dst, void *src);
void pagetable_insert_range(uint64_t *pagetable, void *dst, void *src, int length);
void pagetable_destroy(uint64_t *pagetable);
void *pagetable_translate(uint64_t *pagetable, void *addr);

#endif
