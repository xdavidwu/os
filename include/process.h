#ifndef PROCESS_H
#define PROCESS_H

#include <stddef.h>
#include <stdint.h>

struct process_states {
	void *page;
	size_t image_size;
};

int process_exec(uint8_t *image, size_t image_size);

void process_exit();

#endif
