#include "errno.h"
#include "kio.h"
#include "vfs.h"

#include <stdint.h>

#define FAT32_EBR_SIGNATURE_OFFSET	0x42
#define FAT32_EBR_LABEL_OFFSET	0x47

static int64_t fat32_pread(struct inode *inode, void *buf, size_t count, size_t offset);
static int fat32_getdents(struct inode *inode);
static int fat32_mount(const char *source, struct inode *target, uint32_t flags);

struct vfs_impl fat32_impl = {
	.pread = fat32_pread,
	.getdents = fat32_getdents,
	.mount = fat32_mount,
};

static int fat32_mount(const char *source, struct inode *target, uint32_t flags) {
	if (flags != MS_RDONLY) {
		return -ENOTSUP;
	}
	int err;
	struct fd *dev = vfs_open(source, O_RDONLY, &err);
	if (!dev) {
		return -err;
	}
	uint8_t br[512];
	int res = vfs_pread(dev, br, 512, 0);
	if (res != 512 || (br[FAT32_EBR_SIGNATURE_OFFSET] != 0x28 && br[FAT32_EBR_SIGNATURE_OFFSET] != 0x29)) {
		vfs_close(dev);
		return -EINVAL;
	}
	kputs("FAT32 of label: ");
	for (int i = 0; i < 11; i++) {
		kputc(br[FAT32_EBR_LABEL_OFFSET + i]);
	}
	kputc('\n');
	vfs_close(dev);
	return -ENOTSUP;
}

static int fat32_getdents(struct inode *inode) {
	return -ENOTSUP;
}
static int64_t fat32_pread(struct inode *inode, void *buf, size_t count, size_t offset) {
	return -ENOTSUP;
}

