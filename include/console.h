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
	int (*putc_nonblock)(uint8_t);
	int (*getc_nonblock)();
	void (*set_tx_interrupt)(bool);
	void (*set_rx_interrupt)(bool);
};

enum control_sequence_state { // TODO SS2/3?
	PLAIN,
	ESC_START,
	IN_CSI, // ESC [ detected, currently in CSI
	IN_OSC, // ESC ] detected, currently in OSC
	IN_OSC_ESC, // IN_OSC, last was ESC (ST pending)
	IN_DCS, // ESC P detected, currently in DCS
	IN_DCS_ESC, // IN_DCS, last was ESC (ST pending)
	IN_SOS, // ESC X detected, currently in SOS
	IN_SOS_ESC, // IN_SOS, last was ESC (ST pending)
	IN_PM, // ESC ^ detected, currently in OSC
	IN_PM_ESC, // IN_PM, last was ESC (ST pending)
	IN_APC, // ESC _ detected, currently in OSC
	IN_APC_ESC, // IN_APC, last was ESC (ST pending)
};

#define CONSOLE_BUFFER_SIZE	128

struct console_buffer {
	uint8_t data[CONSOLE_BUFFER_SIZE];
	int head, tail;
	bool full;
};

struct console {
	bool echo;

	int columns, lines;
	int ccolumn, cline;

	struct {
		enum control_sequence_state state;
		struct console_buffer buffer;
	} input;

	struct {
		enum control_sequence_state state;
		struct console_buffer buffer;
	} output;

	const struct console_impl *impl;
};

void cflush_nonblock(struct console *con);
void cinit(struct console *con);
void cputc(struct console *con, char c);
void cputs(struct console *con, const char *str);
char cgetc(const struct console *con);
size_t cgets(struct console *con, char *str, size_t sz);

#endif
