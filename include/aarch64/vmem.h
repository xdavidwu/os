#ifndef AARCH64_VMEM_H
#define AARCH64_VMEM_H

#define PD_TYPE_MASK	0b11
#define PD_TABLE	0b11
#define PD_BLOCK	0b01
#define PD_ACCESS	(1 << 10)
#define PD_USER	(1 << 6)
#define PD_RO	(1 << 7)
#define PD_USER_NX	(1ULL << 54)
#define PD_ADDR_MASK	(((1ULL << 48) - 1) ^ ((1ULL << 12) - 1))

#define MAIR_IDX_DEVICE_nGnRnE	1
#define MAIR_IDX_NORMAL_NOCACHE	0

#define HIGH_MEM_OFFSET	0xffff000000000000

#define ADDR_PGD_IDX(x)	(((x) >> 39) & ((1 << 9) - 1))
#define ADDR_PUD_IDX(x)	(((x) >> 30) & ((1 << 9) - 1))
#define ADDR_PMD_IDX(x)	(((x) >> 21) & ((1 << 9) - 1))
#define ADDR_PTE_IDX(x)	(((x) >> 12) & ((1 << 9) - 1))

// not managed, do not free
#define PAGE_STICKY	(1ULL << 55)
#define PAGE_COW	(1ULL << 56)

#endif
