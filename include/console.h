#ifndef CONSOLE_H
#define CONSOLE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define DEFAULT_COLUMNS	80
#define DEFAULT_LINES	24

#define ESC	"\x1b"
#define CSI	ESC "["

struct console_impl {
	void (*putc)(uint8_t);
	uint8_t (*getc)();
};

struct console {
	bool echo;
	int columns, lines;
	int ccolumn, cline;
	const struct console_impl *impl;
};

void cinit(struct console *con, const struct console_impl *impl);
void cputs(struct console *con, const char *str);
char cgetc(const struct console *con);
size_t cgets(struct console *con, char *str, size_t sz);

#endif
