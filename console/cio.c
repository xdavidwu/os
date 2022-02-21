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
	return c;
}

size_t cgets(const struct console *con, char *str, size_t sz) {
	int i = 0;
	while (i < sz) {
		char c = cgetc(con);
		if (c == '\0') {
			break;
		} else if (c == '\x7f') { // DEL
			if (i && con->echo) {
				cputs(con, "\b \b");
			}
			if (i > 0) {
				i--;
			}
			continue;
		} else if (c == '\n') {
			str[i++] = '\n';
			con->impl->putc(c);
			break;
		}
		if (con->echo) {
			con->impl->putc(c);
		}
		str[i++] = c;
	}
	str[i] = '\0';
	return i;
}
