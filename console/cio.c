#include "console.h"

void cputs(const struct console *con, const char *str) {
	while (*str != '\0') {
		con->putc(*str);
		str++;
	}
}

size_t cgets(const struct console *con, char *str, size_t sz) {
	int i = 0;
	for (; i < sz; i++) {
		char c = con->getc();
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
