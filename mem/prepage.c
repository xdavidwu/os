#include "stdlib.h"

#include <stdint.h>

static uint8_t *heap = (uint8_t *)0x10000;
static int align = 128 / 8;

void *prepage_malloc(size_t size) {
	size = (size + align - 1) / align * align;
	void *const start = heap;
	heap += size;
	return start;
}
