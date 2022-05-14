#ifndef PROCESS_H
#define PROCESS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SIGNAL_MAX	9

enum {
	SIGKILL = 9,
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
};

int process_exec(uint8_t *image, size_t image_size);

void process_exec_inplace(uint8_t *image, size_t image_size);

int process_dup();

void process_exit();

#endif
