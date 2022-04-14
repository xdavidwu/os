#include "stdlib.h"

#include <stdint.h>

void *prepage_heap;
static int align = 128 / 8;

void *prepage_malloc(size_t size) {
	size = (size + align - 1) / align * align;
	void *const start = prepage_heap;
	prepage_heap += size;
	return start;
}
