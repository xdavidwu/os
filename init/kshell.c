#include "fdt.h"
#include "init.h"
#include "kthread.h"
#include "kio.h"
#include "process.h"
#include "stdlib.h"
#include "string.h"
#include "timer.h"
#include "vfs.h"
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

static void ls_print(struct inode *dir, int indent) {
	vfs_ensure_dentries(dir);
	struct dentry *next = dir->entries;
	while (next) {
		for (int i = 0; i < indent; i++) {
			kputc(' ');
		}
		kputs(next->name);
		if ((next->inode->mode & S_IFMT) == S_IFDIR) {
			kputs("/\n");
			ls_print(next->inode, indent + strlen(next->name) + 1);
		} else {
			kputc('\n');
		}
		next = next->next;
	}
}

static void ls() {
	int err;
	struct inode *dir = vfs_get_inode("/", &err);
	ls_print(dir, 0);
}

static void cat() {
	kputs("Filename: ");
	char buf[1024];
	kgets(buf, 1023);
	size_t l = strlen(buf);
	if (buf[l - 1] == '\n') {
		buf[l - 1] = '\0';
	}
	int err;
	struct fd *file = vfs_open(buf, O_RDONLY, &err);
	if (!file) {
		kputs("Cannot open ");
		kputs(buf);
		kputs(": ");
		kputc('0' + err);
		kputc('\n');
		return;
	}
	size_t sz = file->inode->size;
	uint8_t byte;
	while (sz--) {
		vfs_read(file, &byte, 1);
		kputc(byte);
	}
	vfs_close(file);
}

static void exec() {
	kputs("Filename: ");
	char buf[1024];
	kgets(buf, 1023);
	size_t l = strlen(buf);
	if (buf[l - 1] == '\n') {
		buf[l - 1] = '\0';
	}
	int err;
	struct fd *file = vfs_open(buf, O_RDONLY, &err);
	if (!file) {
		kputs("Cannot open ");
		kputs(buf);
		kputs(": ");
		kputc('0' + err);
		kputc('\n');
		return;
	}
	size_t sz = file->inode->size;
	int thr = process_exec(file, sz);
	vfs_close(file);
	kthread_wait(thr);
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
		free(prev);
	}
	prev = addr;
}

static void kputs_func(char *s) {
	kputs(s);
	free(s);
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
	register_timer(time, (void (*)(void *))kputs_func, p, 0);
}

extern void __gppud_delay();

static void prio_task() {
	const char *msg1 = "first\r\n";
	while (*msg1) kconsole->impl->putc(*msg1++);
	kputs("second\n");
	int c = 10000;
	while (c--) {
		__gppud_delay();
	}
	const char *msg3 = "third\r\n";
	while (*msg3) kconsole->impl->putc(*msg3++);
}

static void prio() {
	kputs("A timer has been armed, it will print message in both sync and async.\n"
		"It should be in order first (sync), second (async), third (sync).\n"
		"The timer sync prints in its own low prio task, in first irq,\n"
		"async prints are run by high prio console irq task, in nested irq.\n");
	register_timer(1, prio_task, NULL, 19);
}

static void kthr_thr(void *data) {
	uint64_t n = (uint64_t)data;
	for (int i = 0; i < 10; i++) {
		kputs("Thread: ");
		kputc('0' + n);
		kputc(' ');
		kputc('0' + i);
		kputc('\n');
		kthread_yield();
	}
}

static void kthr() {
	for (uint64_t i = 0; i < 5; i++) {
		kthread_create(kthr_thr, (void *)i);
	}
}

static void help();

static const struct kshell_cmd kshell_cmds[] = {
	{"hello",	"print Hello World!",	hello},
	{"help",	"print this help menu",	help},
	{"reboot",	"reboot the device",	platform_reset},
	{"ls",	"list all files",	ls},
	{"cat",	"print file",	cat},
	{"lsdt",	"print device tree entries",	lsdt},
	{"tmalloc",	"test malloc",	tmalloc},
	{"exec",	"load and exec",	exec},
	{"sleep",	"print something after a few seconds",	sleep},
	{"prio",	"task priority test",	prio},
	{"kthr",	"test kthreads",	kthr},
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
