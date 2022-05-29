#include "vfs.h"

#include "errno.h"
#include "stdlib.h"
#include "string.h"
#include <stdbool.h>

extern struct vfs_impl initrd_impl;
extern struct vfs_impl tmpfs_impl;

struct {
	const char *name;
	struct vfs_impl *impl;
} fs_list[] = {
	{"initrd", &initrd_impl},
	{"tmpfs", &tmpfs_impl},
	{0},
};

struct inode initial_root_node = {
	.mode = S_IFDIR,
};

static struct inode *root = &initial_root_node;

struct path {
	enum {
		PATH_ABSOLUTE,
		PATH_RELATIVE,
	} type;
	int back;
	struct path_component *components;
};

struct path_component {
	const char *name;
	size_t len;
};

static struct path parse_path(const char *path) {
	const char *c = path;
	struct path res = {
		.type = path[0] == '/' ? PATH_ABSOLUTE : PATH_RELATIVE,
		.components = malloc(sizeof(struct path_component) * strlen(path)),
		.back = 0,
	};

	int comp_index = 0;
	while (*c == '/') c++;
	while (*c) {
		const char *comp = c;
		while (*c != '/' && *c) c++;
		size_t len = c - comp;
		if (len == 1 && *comp == '.') {
			goto next;
		} else if (len == 2 && *comp == '.' && comp[1] == '.') {
			if (comp_index) {
				comp_index--;
			} else {
				res.back++;
			}
			goto next;
		}
		res.components[comp_index].name = comp;
		res.components[comp_index].len = len;
		comp_index++;
next:
		while (*c == '/') c++;
	};
	res.components[comp_index].name = NULL;

	return res;
}

int vfs_ensure_dentries(struct inode *node) {
	if ((node->mode & S_IFMT) != S_IFDIR) {
		return -ENOTDIR;
	}
	return node->fs->impl->getdents(node);
}

struct inode *vfs_get_inode(const char *path, int *err) {
	struct path parsed = parse_path(path);
	struct inode *node = root;
	for (int i = 0; parsed.components[i].name; i++) {
		int len = vfs_ensure_dentries(node);
		if (len < 0) {
			*err = -len;
			return NULL;
		}
		bool found = false;
		struct dentry *next = node->entries;
		while (next) {
			if (next->len == parsed.components[i].len &&
					!strncmp(next->name, parsed.components[i].name, next->len)) {
				node = next->inode;
				found = true;
				break;
			}
			next = next->next;
		}
		if (!found) {
			*err = ENOENT;
			return NULL;
		}
	}
	return node;
}

static struct inode *get_inode_parent(const char *path, int *err, char const **name, int *nlen) {
	struct path parsed = parse_path(path);
	struct inode *node = root;
	if (!parsed.components[0].name) {
		*err = EINVAL;
		return NULL;
	}
	int i = 0;
	for (; parsed.components[i + 1].name; i++) {
		int len = vfs_ensure_dentries(node);
		if (len < 0) {
			*err = -len;
			return NULL;
		}
		bool found = false;
		struct dentry *next = node->entries;
		while (next) {
			if (next->len == parsed.components[i].len &&
					!strncmp(next->name, parsed.components[i].name, next->len)) {
				node = next->inode;
				found = true;
				break;
			}
			next = next->next;
		}
		if (!found) {
			*err = ENOENT;
			return NULL;
		}
	}
	*name = parsed.components[i].name;
	*nlen = parsed.components[i].len;
	return node;
}

int vfs_mount(const char *source, const char *target, const char *fs, uint32_t flags) {
	int err = 0;
	struct inode *target_i = vfs_get_inode(target, &err);
	if (!target_i) {
		return -err;
	}

	if ((target_i->mode & S_IFMT) != S_IFDIR) {
		return -ENOTDIR;
	}

	struct vfs_impl *fs_impl = NULL;
	for (int i = 0; fs_list[i].name; i++) {
		if (!strcmp(fs, fs_list[i].name)) {
			fs_impl = fs_list[i].impl;
			break;
		}
	}
	if (!fs_impl) {
		return -ENODEV;
	}

	return fs_impl->mount(source, target_i, flags);
}

struct fd *vfs_open(const char *path, int flags, int *err) {
	struct inode *node = vfs_get_inode(path, err);
	if (!node && (flags & O_CREAT)) {
		node = vfs_mknod(path, S_IFREG, err); // TODO mode
	}
	if (!node) {
		return NULL;
	}
	// quirk: hw test binary does not set accmode
	/*if ((flags & O_ACCMODE) != O_RDONLY && (node->fs->flags & MS_RDONLY)) {
		*err = EROFS;
		return NULL;
	}*/
	if ((node->mode & S_IFMT) != S_IFREG) {
		*err = ENOTSUP;
		return NULL;
	}
	struct fd *res = malloc(sizeof(struct fd));
	res->inode = node;
	res->pos = 0;
	res->flags = flags;
	return res;
}

int vfs_close(struct fd *f) {
	free(f);
	return 0;
}

int vfs_read(struct fd *f, void *buf, size_t count) {
	if ((f->flags & O_ACCMODE) == O_WRONLY) {
		return -EBADF;
	}
	if (f->pos >= f->inode->size) {
		return -EINVAL;
	}
	if (f->pos + count >= f->inode->size) {
		count = f->inode->size - f->pos;
	}
	int res = f->inode->fs->impl->pread(f->inode, buf, count, f->pos);
	if (res > 0) {
		f->pos += res;
	}
	return res;
}

int vfs_write(struct fd *f, const void *buf, size_t count) {
	if ((f->flags & O_ACCMODE) == O_RDONLY) {
		return -EBADF;
	}
	if ((f->flags & O_ACCMODE) != O_RDONLY && (f->inode->fs->flags & MS_RDONLY)) {
		return -EROFS;
	}
	int res = f->inode->fs->impl->pwrite(f->inode, buf, count, f->pos);
	if (res > 0) {
		f->pos += res;
	}
	return res;
}

struct inode *vfs_mknod(const char *name, uint32_t mode, int *err) {
	const char *dname;
	int nlen;
	struct inode *parent = get_inode_parent(name, err, &dname, &nlen);
	if (!parent) {
		return NULL;
	}
	if (parent->fs->flags & MS_RDONLY) {
		*err = EROFS;
		return NULL;
	}
	return parent->fs->impl->mknodat(parent, dname, mode, err);
}
