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

struct console {
	bool echo;

	int columns, lines;
	int ccolumn, cline;

	struct {
		enum control_sequence_state state;
	} input;

	struct {
		enum control_sequence_state state;
	} output;

	const struct console_impl *impl;
};

void cinit(struct console *con, const struct console_impl *impl);
void cputc(struct console *con, char c);
void cputs(struct console *con, const char *str);
char cgetc(const struct console *con);
size_t cgets(struct console *con, char *str, size_t sz);

#endif
