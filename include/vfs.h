#ifndef VFS_H
#define VFS_H

#include <stddef.h>
#include <stdint.h>

#define major(x)	((x) >> 8)
#define minor(x)	((x) & 0xff)
#define makedev(x, y)	(((x) << 8) | y)

enum {
	O_ACCMODE	= 03,
	O_RDONLY	= 00,
	O_WRONLY	= 01,
	O_RDWR	= 02,
	O_CREAT	= 0100,
};

enum {
	S_IFMT	= 0170000,
	S_IFREG	= 0100000,
	S_IFBLK	= 0060000,
	S_IFDIR	= 0040000,
	S_IFCHR	= 0020000,
};

enum {
	MS_RDONLY	= 1,
};

enum {
	SEEK_SET	= 0,
	SEEK_CUR	= 1,
	SEEK_END	= 2,
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
	uint16_t dev;
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
	int64_t (*pwrite)(struct inode *inode, const void *buf, size_t count, size_t offset);
	int (*getdents)(struct inode *inode);
	int (*mount)(const char *source, struct inode *target, uint32_t flags);
	struct inode *(*mknodat)(struct inode *parent, const char *name, uint32_t mode, uint16_t dev, int *err);
};

int vfs_mount(const char *source, const char *target, const char *fs, uint32_t flags);
struct fd *vfs_open(const char *path, int flags, int *err);
int vfs_close(struct fd *i);
int vfs_read(struct fd *f, void *buf, size_t count);
int vfs_write(struct fd *f, const void *buf, size_t count);
int64_t vfs_lseek(struct fd *f, int64_t offset, int whence);
struct inode *vfs_get_inode(const char *path, int *err);
int vfs_ensure_dentries(struct inode *node);
struct inode *vfs_mknod(const char *name, uint32_t mode, uint16_t dev, int *err);
int vfs_ioctl(struct fd *f, uint32_t request, void *data);
int vfs_pread(struct fd *f, void *buf, size_t count, size_t pos);

#endif
