#include "kio.h"
#include <stddef.h>

int console_read(int minor, void *buf, size_t count) {
	char *cbuf = buf;
	for (int a = 0; a < count; a++) {
		cbuf[a] = kgetc();
	}
	return count;
}

int console_write(int minor, const void *buf, size_t count) {
	const char *cbuf = buf;
	for (int a = 0; a < count; a++) {
		kputc(cbuf[a]);
	}
	return count;
}

