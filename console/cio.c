#include "console.h"

void cputs(const struct console *con, const char *str) {
	while (*str != '\0') {
		con->putc(*str);
		str++;
	}
}
