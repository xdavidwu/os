#ifndef CONSOLE_H
#define CONSOLE_H

#include <stddef.h>
#include <stdint.h>

struct console {
	void (*putc)(uint8_t);
	uint8_t (*getc)();
};

void cputs(const struct console *con, const char *str);
size_t cgets(const struct console *con, char *str, size_t sz);

#endif
