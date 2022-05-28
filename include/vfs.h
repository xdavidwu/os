#ifndef VFS_H
#define VFS_H

#include <stddef.h>
#include <stdint.h>

enum {
	O_RDONLY = 00,
};

enum {
	S_IFMT	= 0170000,
	S_IFREG	= 0100000,
	S_IFDIR	= 0040000,
};

enum {
	MS_RDONLY	= 1,
};

struct fs {
	uint32_t flags;
	struct vfs_impl *impl;
};

struct inode {
	uint32_t mode;
	struct fs *fs;
	size_t size;
	struct dentry *entries;
	void *data;
};

struct fd {
	struct inode *inode;
	size_t pos;
	int flags;
};

struct dentry {
	struct inode *inode;
	char *name;
	size_t len;
	struct dentry *next;
};

struct vfs_impl {
	int64_t (*pread)(struct inode *inode, void *buf, size_t count, size_t offset);
	int (*getdents)(struct inode *inode);
	int (*mount)(const char *source, struct inode *target, uint32_t flags);
	int (*mkdirat)(struct inode *parent, const char *name, uint32_t mode);
};

int vfs_mount(const char *source, const char *target, const char *fs, uint32_t flags);
struct fd *vfs_open(const char *path, int flags, int *err);
int vfs_close(struct fd *i);
int vfs_read(struct fd *f, void *buf, size_t count);
struct inode *vfs_get_inode(const char *path, int *err);
int vfs_ensure_dentries(struct inode *node);
int vfs_mkdir(const char *name, uint32_t mode);

#endif
