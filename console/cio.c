#include "console.h"

static void cputs_raw(const struct console *con, const char *str) {
	while (*str != '\0') {
		con->impl->putc(*str);
		str++;
	}
}

void cinit(struct console *con, const struct console_impl *impl) {
	con->echo = true;
	con->lines = DEFAULT_LINES;
	con->columns = DEFAULT_COLUMNS;
	con->cline = 1;
	con->ccolumn = 1;
	con->impl = impl;
	cputs_raw(con, CSI "2J"); // clear all
	cputs_raw(con, CSI "H"); // reset cursor
}

void cputc(struct console *con, char c) {
	switch (c) {
	case '\n':
		if (con->cline == con->lines) { // full
			// scroll, return cursor (TODO: should we CR instead?)
			cputs_raw(con, CSI "S" CSI "G");
		} else {
			con->cline++;
			con->impl->putc('\n');
		}
		con->ccolumn = 1;
		break;
	default:
		if (con->ccolumn == con->columns) { // full
			cputc(con, '\n');
		}
		con->ccolumn++;
		con->impl->putc(c);
		break;
	}
}

void cputs(struct console *con, const char *str) {
	while (*str != '\0') {
		cputc(con, *str);
		str++;
	}
}

char cgetc(const struct console *con) {
	char c = con->impl->getc();
	c = c == '\r' ? '\n' : c;
	return c;
}

size_t cgets(struct console *con, char *str, size_t sz) {
	int i = 0;
	while (i < sz) {
		char c = cgetc(con);
		if (c == '\0') {
			break;
		} else if (c == '\x7f') { // DEL
			if (i && con->echo) {
				cputs_raw(con, "\b \b");
			}
			if (i > 0) {
				i--;
			}
			continue;
		} else if (c == '\n') {
			str[i++] = '\n';
			cputc(con, c);
			break;
		}
		if (con->echo) {
			cputc(con, c);
		}
		str[i++] = c;
	}
	str[i] = '\0';
	return i;
}
