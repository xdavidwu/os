#include "aarch64/registers.h"
#include "aarch64/vmem.h"
#include "bcm2835_mailbox.h"
#include "errno.h"
#include "exceptions.h"
#include "init.h"
#include "kio.h"
#include "kthread.h"
#include "page.h"
#include "process.h"
#include "string.h"
#include "syscall.h"
#include "vfs.h"
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
	int err;
	struct fd *file = vfs_open(name, O_RDONLY, &err);
	if (!file) {
		return -err;
	}
	size_t sz = file->inode->size;
	process_exec_inplace(file, sz);
	return 0;
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

static int first_empty_fd(struct process_states *states) {
	for (int i = 0 ; i < FD_MAX; i++) {
		if (!states->fds[i]) {
			return i;
		}
	}
	return -EMFILE;
}

static reg_t open(const char *pathname, int flags) {
	struct kthread_states *states;
	__asm__ ("mrs %0, tpidr_el1" : "=r" (states));
	struct process_states *process = states->data;
	int f = first_empty_fd(process);
	if (f < 0) {
		return f;
	}
	int err;
	// FIXME: quirk: homework test binary does not set access flags
	process->fds[f] = vfs_open(pathname, flags | O_RDWR, &err);
	if (!process->fds[f]) {
		return -err;
	}
	return f;
}

static reg_t close(int fd) {
	struct kthread_states *states;
	__asm__ ("mrs %0, tpidr_el1" : "=r" (states));
	struct process_states *process = states->data;
	if (fd >= FD_MAX || fd < 0 || !process->fds[fd]) {
		return -EBADF;
	}
	vfs_close(process->fds[fd]);
	process->fds[fd] = NULL;
	return 0;
}

static reg_t write(int fd, const void *buf, size_t count) {
	struct kthread_states *states;
	__asm__ ("mrs %0, tpidr_el1" : "=r" (states));
	struct process_states *process = states->data;
	if (fd >= FD_MAX || fd < 0 || !process->fds[fd]) {
		return -EBADF;
	}
	return vfs_write(process->fds[fd], buf, count);
}

static reg_t read(int fd, void *buf, size_t count) {
	struct kthread_states *states;
	__asm__ ("mrs %0, tpidr_el1" : "=r" (states));
	struct process_states *process = states->data;
	if (fd >= FD_MAX || fd < 0 || !process->fds[fd]) {
		return -EBADF;
	}
	int res = vfs_read(process->fds[fd], buf, count);
	return res;
}

static reg_t mkdir(const char *pathname, uint32_t mode) {
	int err;
	if (mode > 0777) {
		return -EINVAL;
	}
	if (!vfs_mknod(pathname, mode | S_IFDIR, &err)) {
		return -err;
	}
	return 0;
}

static reg_t mount(const char *src, const char *target, const char *filesystem,
		uint32_t flags, const void *data) {
	return vfs_mount(src, target, filesystem, flags);
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
	mmap, // 10
	open,
	close,
	write,
	read,
	mkdir,
	mount,
	syscall_reserved,
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
