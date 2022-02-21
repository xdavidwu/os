#ifndef CONSOLE_H
#define CONSOLE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct console_impl {
	void (*putc)(uint8_t);
	uint8_t (*getc)();
};

struct console {
	bool echo;
	const struct console_impl *impl;
};

void cputs(const struct console *con, const char *str);
char cgetc(const struct console *con);
size_t cgets(const struct console *con, char *str, size_t sz);

#endif
