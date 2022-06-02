#include "errno.h"
#include "vfs.h"
#include "cpio.h"
#include "init.h"
#include <stddef.h>
#include <stdint.h>
#include "stdlib.h"
#include "string.h"

/*
 * Assumption: entries in order
 * first is a .
 * the rest are without ./
 */

static int64_t initrd_pread(struct inode *inode, void *buf, size_t count, size_t offset);
static int initrd_getdents(struct inode *inode);
static int initrd_mount(const char *source, struct inode *target, uint32_t flags);

struct vfs_impl initrd_impl = {
	.pread = initrd_pread,
	.getdents = initrd_getdents,
	.mount = initrd_mount,
};

static int initrd_mount(const char *source, struct inode *target, uint32_t flags) {
	if (flags != MS_RDONLY) {
		return -ENOTSUP;
	}
	uint8_t *cpio = initrd_start;
	if (cpio_is_end(cpio)) {
		return -ENOENT;
	}
	struct cpio_newc_header *cpio_header =
		(struct cpio_newc_header *) cpio;
	uint32_t namesz = cpio_get_uint32(cpio_header->c_namesize);
	if (namesz != 2 || *cpio_get_name(cpio) != '.') {
		return -ENOENT;
	}
	target->size = 0;
	target->entries = NULL;
	target->data = cpio;
	target->fs = malloc(sizeof(struct fs));
	target->fs->flags = flags;
	target->fs->impl = &initrd_impl;
	return 0;
}

static int initrd_getdents(struct inode *inode) {
	if (inode->entries) {
		return inode->size;
	}
	const char *name = cpio_get_name(inode->data);
	bool is_root = !strcmp(name, ".");
	int len = is_root ? 0 : strlen(name);
	struct dentry **next = &inode->entries;
	uint8_t *cpio = inode->data;
	uint32_t namesz, filesz;
	int count = 0;
	struct cpio_newc_header *cpio_header =
		(struct cpio_newc_header *) cpio;
	namesz = cpio_get_uint32(cpio_header->c_namesize);
	filesz = cpio_get_uint32(cpio_header->c_filesize);
	while ((cpio = cpio_next_entry(cpio, namesz, filesz))) {
		struct cpio_newc_header *cpio_header =
			(struct cpio_newc_header *) cpio;
		namesz = cpio_get_uint32(cpio_header->c_namesize);
		filesz = cpio_get_uint32(cpio_header->c_filesize);
		const char *cname = cpio_get_name(cpio);
		if (is_root || !strncmp(cname, name, len)) {
			if (!is_root && cname[len] != '/') {
				continue;
			}
			const char *trimmed = is_root ? cname : &cname[len + 1];
			const char *ptr = trimmed;
			bool next_level = false;
			while (*ptr) {
				if (*ptr == '/') {
					next_level = true;
					break;
				}
				ptr++;
			}
			if (next_level) {
				continue;
			}
			*next = malloc(sizeof(struct dentry));
			(*next)->name = strdup(trimmed);
			(*next)->len = strlen((*next)->name);
			(*next)->inode = malloc(sizeof(struct inode));
			(*next)->inode->fs = inode->fs;
			(*next)->inode->data = cpio;
			(*next)->inode->size = filesz;
			(*next)->inode->mode = cpio_get_uint32(cpio_header->c_mode);
			(*next)->inode->entries = NULL;
			next = &(*next)->next;
			count++;
		} else {
			break;
		}
	}
	*next = NULL;
	inode->size = count;
	return count;
}

int64_t initrd_pread(struct inode *inode, void *buf, size_t count, size_t offset) {
	if (offset + count > inode->size) {
		count = inode->size - offset;
	}
	struct cpio_newc_header *header = inode->data;
	uint32_t namesz = cpio_get_uint32(header->c_namesize);
	uint8_t *c = buf;
	uint8_t *s = cpio_get_file(inode->data, namesz) + offset;
	size_t sz = count;
	while (sz--) {
		*c++ = *s++;
	}
	return count;
}
