#include "fdt.h"
#include "init.h"
#include "kio.h"
#include "kshell.h"
#include "kthread.h"
#include "string.h"
#include "vfs.h"

extern void kthread_loop();
extern void sdcard_probe();

void main() {
	int err;
	vfs_mount("", "/", "tmpfs", 0);
	vfs_mknod("/initramfs", S_IFDIR | 0644, 0, &err);
	vfs_mknod("/dev", S_IFDIR | 0644, 0, &err);
	vfs_mknod("/dev/uart", S_IFCHR | 0644, makedev(0, 0), &err);
	vfs_mknod("/dev/framebuffer", S_IFBLK | 0644, makedev(0, 0), &err);
	vfs_mknod("/sdcard", S_IFDIR | 0644, 0, &err);
	sdcard_probe();
	int res = vfs_mount("", "/initramfs", "initrd", MS_RDONLY);
	if (res < 0) {
		kputs("Failed to mount initrd: ");
		kputc('0' + -res);
		kputc('\n');
	}
	res = vfs_mount("/dev/sdcard0p1", "/sdcard", "fat32", MS_RDONLY);
	if (res < 0) {
		kputs("Failed to mount sdcard: ");
		kputc('0' + -res);
		kputc('\n');
	}
	kthread_create(kshell, NULL);
	kthread_loop();
}
