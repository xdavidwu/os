#include "aarch64/vmem.h"
#include "errno.h"
#include "vfs.h"
#include "init.h"
#include "page.h"
#include <stddef.h>
#include <stdint.h>
#include "stdlib.h"
#include "string.h"

static int64_t tmpfs_pread(struct inode *inode, void *buf, size_t count, size_t offset);
static int64_t tmpfs_pwrite(struct inode *inode, const void *buf, size_t count, size_t offset);
static int tmpfs_getdents(struct inode *inode);
static int tmpfs_mount(const char *source, struct inode *target, uint32_t flags);
static struct inode *tmpfs_mknodat(struct inode *parent, const char *name, uint32_t mode, int *err);

struct vfs_impl tmpfs_impl = {
	.pread = tmpfs_pread,
	.pwrite = tmpfs_pwrite,
	.getdents = tmpfs_getdents,
	.mount = tmpfs_mount,
	.mknodat = tmpfs_mknodat,
};

static int tmpfs_mount(const char *source, struct inode *target, uint32_t flags) {
	target->fs = malloc(sizeof(struct fs));
	target->fs->flags = flags;
	target->fs->impl = &tmpfs_impl;
	return 0;
}

static int tmpfs_getdents(struct inode *inode) {
	return inode->size;
}

int64_t tmpfs_pread(struct inode *inode, void *buf, size_t count, size_t offset) {
	uint8_t *c = buf;
	uint8_t *s = inode->data + offset;
	size_t sz = count;
	while (sz--) {
		*c++ = *s++;
	}
	return count;
}

int64_t tmpfs_pwrite(struct inode *inode, const void *buf, size_t count, size_t offset) {
	if (!inode->data) {
		inode->data = HIGH_MEM_OFFSET + page_alloc(1); // TODO bigger file
	}
	const uint8_t *c = buf;
	uint8_t *s = inode->data + offset;
	size_t sz = count;
	while (sz--) {
		*s++ = *c++;
	}
	if (offset + count > inode->size) {
		inode->size = offset + count;
	}
	return count;
}

static struct inode *tmpfs_mknodat(struct inode *parent, const char *name, uint32_t mode, int *err) {
	struct dentry *bak = parent->entries;
	parent->entries = malloc(sizeof(struct dentry));
	parent->entries->next = bak;
	parent->entries->name = strdup(name);
	parent->entries->len = strlen(name);
	parent->entries->inode = malloc(sizeof(struct inode));
	parent->entries->inode->mode = mode;
	parent->entries->inode->size = 0;
	parent->entries->inode->fs = parent->fs;
	parent->size++;
	return parent->entries->inode;
}
