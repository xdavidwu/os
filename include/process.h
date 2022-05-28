#ifndef PROCESS_H
#define PROCESS_H

#include "vfs.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SIGNAL_MAX	11
#define FD_MAX	64

enum {
	SIGKILL = 9,
	SIGSEGV = 11,
};

struct process_image {
	void *page;
	size_t size;
};

struct process_states {
	struct process_image image;
	uint32_t pending_signals;
	void (*signal_handlers[SIGNAL_MAX + 1])(int);
	uint64_t presignal_sp;
	bool in_signal;
	struct trapframe *trapframe;
	uint64_t *pagetable;
	struct fd *fds[FD_MAX];
};

int process_exec(struct fd *f, size_t image_size);

void process_exec_inplace(struct fd *f, size_t image_size);

int process_dup();

void process_exit();

#endif
