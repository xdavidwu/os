#include "cpio.h"
#include "init.h"
#include "kio.h"
#include "string.h"
#include "pm.h"
#include <stdint.h>

struct kshell_cmd {
	const char *cmd;
	const char *help;
	void (*handler)();
};

static void hello() {
	kputs("Hello World!\n");
}

static void ls() {
	uint8_t *cpio = initrd_start;
	if (cpio_is_end(cpio)) {
		return;
	}
	uint32_t namesz, filesz;
	do {
		struct cpio_newc_header *cpio_header =
			(struct cpio_newc_header *) cpio;
		namesz = cpio_get_uint32(cpio_header->c_namesize);
		filesz = cpio_get_uint32(cpio_header->c_filesize);
		kputs(cpio_get_name(cpio));
		kputs("\n");
	} while ((cpio = cpio_next_entry(cpio, namesz, filesz)));
}

static void help();

static const struct kshell_cmd kshell_cmds[] = {
	{"hello",	"print Hello World!",	hello},
	{"help",	"print this help menu",	help},
	{"reboot",	"reboot the device",	platform_reset},
	{"ls",	"list entries in initrd cpio",	ls},
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
		bool recognized = false;
		for (int i = 0; kshell_cmds[i].cmd; i++) {
			if (!strcmp(buf, kshell_cmds[i].cmd)) {
				kshell_cmds[i].handler();
				recognized = true;
				break;
			}
		}
		if (!recognized) {
			kputs("Unrecognized command: ");
			kputs(buf);
			kputs("\n");
		}
	}
}
