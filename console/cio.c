#include "console.h"
#include "exceptions.h"
#include "kthread.h"

static void cputc_raw(struct console *con, uint8_t c) {
	DISABLE_INTERRUPTS();
	if (con->output.buffer.full) {
		con->impl->putc(con->output.buffer.data[con->output.buffer.head]);
		con->output.buffer.head++;
		if (con->output.buffer.head == CONSOLE_BUFFER_SIZE) {
			con->output.buffer.head = 0;
		}
	}
	con->output.buffer.data[con->output.buffer.tail++] = c;
	if (con->output.buffer.tail == CONSOLE_BUFFER_SIZE) {
		con->output.buffer.tail = 0;
	}
	if (con->output.buffer.head) {
		con->output.buffer.full = (con->output.buffer.tail ==
			con->output.buffer.head - 1);
	} else {
		con->output.buffer.full = (con->output.buffer.tail ==
			CONSOLE_BUFFER_SIZE - 1);
	}
	con->impl->set_tx_interrupt(true);
	ENABLE_INTERRUPTS();
}

static void cputs_raw(struct console *con, const char *str) {
	while (*str != '\0') {
		cputc_raw(con, *str);
		str++;
	}
}

void cflush_nonblock(struct console *con) {
	DISABLE_INTERRUPTS();
	con->impl->set_tx_interrupt(false);
	while (true) {
		if (con->output.buffer.head == con->output.buffer.tail) {
			ENABLE_INTERRUPTS();
			return;
		}
		if (con->impl->putc_nonblock(
				con->output.buffer.data[con->output.buffer.head]) < 0) {
			con->impl->set_tx_interrupt(true);
			ENABLE_INTERRUPTS();
			return;
		}
		con->output.buffer.full = false;
		con->output.buffer.head++;
		if (con->output.buffer.head == CONSOLE_BUFFER_SIZE) {
			con->output.buffer.head = 0;
		}
	}
}

void cinit(struct console *con) {
	con->echo = true;
	con->lines = DEFAULT_LINES;
	con->columns = DEFAULT_COLUMNS;
	con->cline = 1;
	con->ccolumn = 1;
	con->input.state = PLAIN;
	con->output.state = PLAIN;
	con->input.buffer.head = con->input.buffer.tail = 0;
	con->output.buffer.head = con->output.buffer.tail = 0;
	cputs_raw(con, "\r\n"); // hack around missing first bytes
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
			cputs_raw(con, "\r\n");
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
			cputc_raw(con, '\b');
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
			cputc_raw(con, '\t');
		}
		break;
	default:
		if (con->ccolumn == con->columns + 1) { // full
			cputc(con, '\n');
		}
		con->ccolumn++;
		cputc_raw(con, c);
		break;
	}
}

void cputs(struct console *con, const char *str) {
	while (*str != '\0') {
		cputc(con, *str);
		str++;
	}
}

void cconsume_nonblock(struct console *con) {
	int c;
	while (!con->input.buffer.full) {
		if ((c = con->impl->getc_nonblock()) < 0) {
			con->impl->set_rx_interrupt(true);
			return;
		}
		DISABLE_INTERRUPTS();
		con->input.buffer.data[con->input.buffer.tail++] = c;
		if (con->input.buffer.tail == CONSOLE_BUFFER_SIZE) {
			con->input.buffer.tail = 0;
		}
		if (con->input.buffer.head) {
			con->input.buffer.full = (con->input.buffer.tail ==
				con->input.buffer.head - 1);
		} else {
			con->input.buffer.full = (con->input.buffer.tail ==
				CONSOLE_BUFFER_SIZE - 1);
		}
		ENABLE_INTERRUPTS();
	}
}

char cgetc(struct console *con) {
	char c;
	while (con->input.buffer.head == con->input.buffer.tail) {
		kthread_yield();
	}
	DISABLE_INTERRUPTS();
	c = con->input.buffer.data[con->input.buffer.head];
	con->input.buffer.full = false;
	con->input.buffer.head++;
	if (con->input.buffer.head == CONSOLE_BUFFER_SIZE) {
		con->input.buffer.head = 0;
	}
	con->impl->set_rx_interrupt(true);
	ENABLE_INTERRUPTS();
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
