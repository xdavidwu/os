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
	con->input.state = PLAIN;
	con->output.state = PLAIN;
	cputs_raw(con, CSI "2J" CSI "H"); // clear all, reset cursor
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
	case '\b':
		con->ccolumn--;
		if (!con->ccolumn) { // consume last column on prev line
			if (con->cline == 1) { // 1,1 noop
				con->ccolumn = 1;
				return;
			}
			con->ccolumn = con->columns;
			con->cline--;
			// up, last char, FIXME hardcoded columns
			cputs_raw(con, CSI "A" CSI "80G");
		} else {
			con->impl->putc('\b');
		}
		break;
	case '\t':
		if (con->ccolumn == con->columns + 1) { // full
			cputc(con, '\n');
			// tab at tail, don't wrap
			return;
		}
		con->ccolumn = (con->ccolumn / 8 + 1) * 8 + 1;
		if (con->ccolumn > con->columns) { // to last column
			con->ccolumn = con->columns;
			// FIXME hardcoded columns
			cputs_raw(con, CSI "80G");
		} else {
			// TODO: should we space instead?
			con->impl->putc('\t');
		}
		break;
	default:
		if (con->ccolumn == con->columns + 1) { // full
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
		} else if (c == '\x7f') { // DEL TODO in control seq?
			if (i && con->echo) {
				cputs(con, "\b \b");
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
		switch (con->input.state) {
		case PLAIN:
			if (c == '\x1b') {
				con->input.state = ESC_START;
				continue;
			}
			break;
		case ESC_START:
			switch (c) {
			case '[':
				con->input.state = IN_CSI;
				continue;
			}
			// unrecognized escape seq
			con->input.state = PLAIN;
			cputs(con, "^["); // output, escaped
			str[i++] = '^';
			if (i < sz) {
				str[i++] = '[';
			}
			continue;
		case IN_CSI: // TODO record / decode
			if (c >= 0x40 && c <= 0x7e) { // final byte
				con->input.state = PLAIN;
				continue;
			}
			continue;
		default:
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
