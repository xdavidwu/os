#include "stdlib.h"

#include <stdint.h>

static uint8_t *heap = (uint8_t *)0x10000;

void *malloc(size_t size) {
	void *start = heap;
	heap += size;
	return start;
}
