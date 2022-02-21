#include "kio.h"
#include "string.h"

struct kshell_cmd {
	const char *cmd;
	const char *help;
	void (*handler)();
};

static void hello() {
	kputs("Hello World!\n");
}

static void help();

static const struct kshell_cmd kshell_cmds[] = {
	{"hello",	"print Hello World!",	hello},
	{"help",	"print this help menu",	help},
	{0},
};

static void help() {
	for (int i = 0; kshell_cmds[i].cmd; i++) {
		kputs(kshell_cmds[i].cmd);
		kputs("\t: ");
		kputs(kshell_cmds[i].help);
		kputs("\n");
	}
}

void kshell() {
	while (1) {
		kputs("# ");
		char buf[1024];
		kgets(buf, 1023);
		size_t l = strlen(buf);
		if (buf[l - 1] == '\n') {
			buf[l - 1] = '\0';
		}
		for (int i = 0; kshell_cmds[i].cmd; i++) {
			if (!strcmp(buf, kshell_cmds[i].cmd)) {
				kshell_cmds[i].handler();
				break;
			}
		}
	}
}
