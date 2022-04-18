#include "aarch64/registers.h"
#include "kio.h"
#include "syscall.h"
#include <stddef.h>

static reg_t syscall_reserved() {
	kputs("Unrecognized syscall\n");
	return 0;
}

static reg_t cwrite(reg_t rbuf, reg_t rsize) {
	const char *buf = (const char *)rbuf;
	size_t size = (size_t) rsize;
	for (int a = 0; a < size; a++) {
		kputc(buf[a]);
	}
	return rsize;
}

static reg_t cread(reg_t rbuf, reg_t rsize) {
	char *buf = (char *)rbuf;
	size_t size = (size_t) rsize;
	for (int a = 0; a < size; a++) {
		buf[a] = kgetc();
	}
	return rsize;
}

#define SYSCALLS_COUNT	3
static reg_t (*syscalls[])(reg_t, reg_t) = {
	syscall_reserved,
	cread,
	cwrite,
};


void syscall(struct trapframe *trapframe) {
	if (trapframe->REGISTER_SYSCALL_NUM >= SYSCALLS_COUNT) {
		kputs("Unrecognized syscall\n");
	} else {
		trapframe->REGISTER_SYSCALL_RET =
			syscalls[trapframe->REGISTER_SYSCALL_NUM](
				trapframe->REGISTER_SYSCALL_P1,
				trapframe->REGISTER_SYSCALL_P2);
	}
}
