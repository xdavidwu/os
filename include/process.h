#ifndef PROCESS_H
#define PROCESS_H

#include <stddef.h>
#include <stdint.h>

struct process_image {
	void *page;
	size_t size;
	int ref;
};

struct process_states {
	void *page;
	struct process_image *image;
};


int process_exec(uint8_t *image, size_t image_size);

void process_exec_inplace(uint8_t *image, size_t image_size);

int process_dup();

void process_exit();

#endif
