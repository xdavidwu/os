#ifndef AARCH64_REGISTERS_H
#define AARCH64_REGISTERS_H

#include <stdint.h>

typedef uint64_t reg_t;

struct trapframe {
	reg_t x0, x1;
	reg_t x2, x3;
	reg_t x4, x5;
	reg_t x6, x7;
	reg_t x8, x9;
	reg_t x10, x11;
	reg_t x12, x13;
	reg_t x14, x15;
	reg_t x16, x17;
	reg_t x18, x30;
	reg_t spsr_el1, elr_el1;
};

#define REGISTER_SYSCALL_NUM	x8
#define REGISTER_SYSCALL_RET	x0
#define REGISTER_SYSCALL_P1	x0
#define REGISTER_SYSCALL_P2	x1

#define ESR_EL1_EC_MASK	0xfc000000
#define ESR_EL1_EC_OFFSET	26
#define ESR_EL1_EC_SVC_AARCH64	(0b010101 << ESR_EL1_EC_OFFSET)

#endif
