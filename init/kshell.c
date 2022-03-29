#include "cpio.h"
#include "fdt.h"
#include "init.h"
#include "kio.h"
#include "stdlib.h"
#include "string.h"
#include "timer.h"
#include "pm.h"
#include <stdint.h>

static uint8_t *const userspace_start_addr = (uint8_t *) 0x100000;
extern void exec_user(uint8_t *addr);

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

static void cat() {
	kputs("Filename: ");
	char buf[1024];
	kgets(buf, 1023);
	size_t l = strlen(buf);
	if (buf[l - 1] == '\n') {
		buf[l - 1] = '\0';
	}
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
		if (!strcmp(cpio_get_name(cpio), buf)) {
			cpio = cpio_get_file(cpio, namesz);
			while (filesz--) {
				kputc(*cpio);
				cpio++;
			}
			break;
		}
	} while ((cpio = cpio_next_entry(cpio, namesz, filesz)));
}

static void exec() {
	kputs("Filename: ");
	char buf[1024];
	kgets(buf, 1023);
	size_t l = strlen(buf);
	if (buf[l - 1] == '\n') {
		buf[l - 1] = '\0';
	}
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
		if (!strcmp(cpio_get_name(cpio), buf)) {
			cpio = cpio_get_file(cpio, namesz);
			uint8_t *ptr = userspace_start_addr;
			while (filesz--) {
				*ptr++ = *cpio++;
			}
			exec_user(userspace_start_addr);
			break;
		}
	} while ((cpio = cpio_next_entry(cpio, namesz, filesz)));
}

static bool print_fdt(uint32_t *token) {
	kputs(fdt_full_path);
	kputc('\n');
	return false;
}

static void lsdt() {
	fdt_traverse(print_fdt);
}

static void tmalloc() {
	static int c = 0;
	static char *prev = NULL;
	c++;
	char *addr = malloc(15);
	strcpy(addr, "test malloc 0\n");
	addr[12] += c % 10;
	kputs(addr);
	if (prev) {
		kputs("prev: ");
		kputs(prev);
	}
	prev = addr;
}

static void kputs_func(char *s) {
	kputs(s);
}

static void sleep() {
	kputs("time: ");
	char buf[1024];
	kgets(buf, 1023);
	int time = 0;
	char *ptr = buf;
	while (*ptr && *ptr != '\n') {
		if (*ptr >= '0' && *ptr <= '9') {
			time *= 10;
			time += *ptr - '0';
		}
		ptr++;
	}
	kputs("string: ");
	kgets(buf, 1023);
	int l = strlen(buf);
	char *p = malloc(l + 1);
	strcpy(p, buf);
	register_timer(time, (void (*)(void *))kputs_func, p);
}

static void help();

static const struct kshell_cmd kshell_cmds[] = {
	{"hello",	"print Hello World!",	hello},
	{"help",	"print this help menu",	help},
	{"reboot",	"reboot the device",	platform_reset},
	{"ls",	"list entries from initrd cpio",	ls},
	{"cat",	"print file from initrd cpio",	cat},
	{"lsdt",	"print device tree entries",	lsdt},
	{"tmalloc",	"test malloc",	tmalloc},
	{"exec",	"load from initrd cpio and exec",	exec},
	{"sleep",	"print something after a few seconds",	sleep},
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
