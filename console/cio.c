#include "console.h"

void cputs(const struct console *con, const char *str) {
	while (*str != '\0') {
		con->impl->putc(*str);
		str++;
	}
}

char cgetc(const struct console *con) {
	char c = con->impl->getc();
	c = c == '\r' ? '\n' : c; // should we really do this?
	if (con->echo) {
		con->impl->putc(c);
	}
	return c;
}

size_t cgets(const struct console *con, char *str, size_t sz) {
	int i = 0;
	for (; i < sz; i++) {
		char c = cgetc(con);
		if (c == '\0') {
			break;
		} else if (c == '\n') {
			str[i++] = '\n';
			break;
		}
		str[i] = c;
	}
	str[i] = '\0';
	return i;
}
