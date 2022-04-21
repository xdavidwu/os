#include "aarch64/registers.h"
#include "bcm2835_mailbox.h"
#include "cpio.h"
#include "errno.h"
#include "init.h"
#include "kio.h"
#include "kthread.h"
#include "process.h"
#include "string.h"
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

static reg_t getpid() {
	struct kthread_states *states;
	__asm__ ("mrs %0, tpidr_el1" : "=r" (states));
	return states->pid;
}

static reg_t exec(reg_t rname, reg_t unused) {
	const char *name = (const char *)rname;
	uint8_t *cpio = initrd_start;
	if (cpio_is_end(cpio)) {
		return -ENOENT;
	}
	uint32_t namesz, filesz;
	do {
		struct cpio_newc_header *cpio_header =
			(struct cpio_newc_header *) cpio;
		namesz = cpio_get_uint32(cpio_header->c_namesize);
		filesz = cpio_get_uint32(cpio_header->c_filesize);
		if (!strcmp(cpio_get_name(cpio), name)) {
			cpio = cpio_get_file(cpio, namesz);
			process_exec_inplace(cpio, filesz);
			return 0;
		}
	} while ((cpio = cpio_next_entry(cpio, namesz, filesz)));
	return -ENOENT;
}

static reg_t (*syscalls[])(reg_t, reg_t) = {
	getpid,
	cread,
	cwrite,
	exec,
	(reg_t (*)(reg_t, reg_t))process_dup,
	(reg_t (*)(reg_t, reg_t))process_exit,
	(reg_t (*)(reg_t, reg_t))mbox_call,
};

void syscall(struct trapframe *trapframe) {
	if (trapframe->REGISTER_SYSCALL_NUM >= sizeof(syscalls) / sizeof(syscalls[0])) {
		kputs("Unrecognized syscall\n");
	} else {
		trapframe->REGISTER_SYSCALL_RET =
			syscalls[trapframe->REGISTER_SYSCALL_NUM](
				trapframe->REGISTER_SYSCALL_P1,
				trapframe->REGISTER_SYSCALL_P2);
	}
}
