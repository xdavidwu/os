#include "aarch64/registers.h"
#include "aarch64/vmem.h"
#include "bcm2835_mailbox.h"
#include "cpio.h"
#include "errno.h"
#include "exceptions.h"
#include "init.h"
#include "kio.h"
#include "kthread.h"
#include "page.h"
#include "process.h"
#include "string.h"
#include "syscall.h"
#include "vmem.h"
#include <stddef.h>
#include <stdint.h>

static reg_t syscall_reserved() {
	kputs("Unrecognized syscall\n");
	return 0;
}

static reg_t cwrite(reg_t rbuf, reg_t rsize) {
	struct kthread_states *states;
	__asm__ ("mrs %0, tpidr_el1" : "=r" (states));
	struct process_states *process = states->data;
	const char *buf = pagetable_translate(process->pagetable, (void *)rbuf) + HIGH_MEM_OFFSET;
	size_t size = (size_t) rsize;
	for (int a = 0; a < size; a++) {
		kputc(buf[a]);
	}
	return rsize;
}

static reg_t cread(reg_t rbuf, reg_t rsize) {
	struct kthread_states *states;
	__asm__ ("mrs %0, tpidr_el1" : "=r" (states));
	struct process_states *process = states->data;
	char *buf = pagetable_translate(process->pagetable, (void *)rbuf) + HIGH_MEM_OFFSET;
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

static reg_t signal(reg_t rsig, reg_t rhandler) {
	void (*handler)(int) = (void (*)(int))rhandler;
	if (rsig > SIGNAL_MAX) {
		return -1;
	}
	struct kthread_states *states;
	__asm__ ("mrs %0, tpidr_el1" : "=r" (states));
	struct process_states *process = states->data;
	process->signal_handlers[rsig] = handler;
	return 0;
}

extern struct kthread_states *runq;

static reg_t kill(reg_t rpid, reg_t rsig) {
	if (rsig > SIGNAL_MAX) {
		return -1;
	}
	bool found = false;
	DISABLE_INTERRUPTS();
	struct kthread_states *states = runq;
	while (states) {
		if (states->pid == rpid) {
			found = true;
			// TODO check type
			struct process_states *process = states->data;
			process->pending_signals |= (1 << rsig);
			break;
		}
		states = states->next;
	}
	__asm__ ("mrs %0, tpidr_el1" : "=r" (states));
	if (states->pid == rpid) {
		found = true;
		struct process_states *process = states->data;
		process->pending_signals |= (1 << rsig);
	}
	ENABLE_INTERRUPTS();
	if (!found) {
		return -1;
	}
	return 0;
}

static reg_t skill(reg_t rpid, reg_t unused) {
	return kill(rpid, SIGKILL);
}

extern void restore_after_signal(uint64_t sp);

static reg_t sigreturn() {
	struct kthread_states *states;
	__asm__ ("mrs %0, tpidr_el1" : "=r" (states));
	struct process_states *process = states->data;
	restore_after_signal(process->presignal_sp);
	return 0;
}

static reg_t mbox_call_user(reg_t rchannel, reg_t raddr) {
	struct kthread_states *states;
	__asm__ ("mrs %0, tpidr_el1" : "=r" (states));
	struct process_states *process = states->data;
	void *pa = pagetable_translate(process->pagetable, (void *)raddr);
	return mbox_call(rchannel, pa);
}

#define MAP_ANON	0x20

enum {
	PROT_READ = 1 << 0,
	PROT_WRITE = 1 << 1,
	PROT_EXEC = 1 << 2,
};

static reg_t mmap(reg_t raddr, reg_t rlen, reg_t rprot, reg_t rflags) {
	struct kthread_states *states;
	__asm__ ("mrs %0, tpidr_el1" : "=r" (states));
	struct process_states *process = states->data;
	if (!(rflags & MAP_ANON) || !rlen || !rprot) {
		return (reg_t)NULL;
	}
	int sz = (rlen + PAGE_UNIT - 1) / PAGE_UNIT;
	void *fit = pagetable_find_fit(process->pagetable, (void *)raddr, sz);
	int perm = 0;
	if (rprot & PROT_WRITE) {
		perm |= PAGETABLE_USER_W;
	}
	if (rprot & PROT_EXEC) {
		perm |= PAGETABLE_USER_X;
	}
	pagetable_ondemand_range(process->pagetable, perm, fit, sz);
	return (reg_t)fit;
}

static reg_t (*syscalls[])() = {
	getpid,
	cread,
	cwrite,
	exec,
	(reg_t (*)())process_dup,
	(reg_t (*)())process_exit,
	mbox_call_user,
	skill,
	signal,
	kill,
	mmap,
	sigreturn,
};

void syscall(struct trapframe *trapframe) {
	if (trapframe->REGISTER_SYSCALL_NUM >= sizeof(syscalls) / sizeof(syscalls[0])) {
		kputs("Unrecognized syscall\n");
	} else {
		trapframe->REGISTER_SYSCALL_RET =
			syscalls[trapframe->REGISTER_SYSCALL_NUM](
				trapframe->REGISTER_SYSCALL_P1,
				trapframe->REGISTER_SYSCALL_P2,
				trapframe->REGISTER_SYSCALL_P3,
				trapframe->REGISTER_SYSCALL_P4);
	}
}
