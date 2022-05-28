#include "fdt.h"
#include "init.h"
#include "kio.h"
#include "kshell.h"
#include "kthread.h"
#include "string.h"
#include "vfs.h"

extern void kthread_loop();

void main() {
	vfs_mount("", "/", "tmpfs", 0);
	vfs_mkdir("/initramfs", 0644);
	int res = vfs_mount("", "/initramfs", "initrd", MS_RDONLY);
	if (res < 0) {
		kputs("Failed to mount initrd: ");
		kputc('0' + -res);
		kputc('\n');
	}
	kthread_create(kshell, NULL);
	kthread_loop();
}
