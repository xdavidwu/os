#ifndef AARCH64_VMEM_H
#define AARCH64_VMEM_H

#define PD_TABLE	0b11
#define PD_BLOCK	0b01
#define PD_ACCESS	(1 << 10)

#define MAIR_IDX_DEVICE_nGnRnE	0
#define MAIR_IDX_NORMAL_NOCACHE	1

#define HIGH_MEM_OFFSET	0xffff000000000000

#endif
