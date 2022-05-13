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
	int ref;
};

struct process_states {
	void *page;
	struct process_image *image;
	uint32_t pending_signals;
	void (*signal_handlers[SIGNAL_MAX + 1])(int);
	uint64_t presignal_sp;
	void *signal_stack;
	bool in_signal;
};

int process_exec(uint8_t *image, size_t image_size);

void process_exec_inplace(uint8_t *image, size_t image_size);

int process_dup();

void process_exit();

#endif
