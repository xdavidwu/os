#include "vfs.h"

#include "errno.h"
#include "stdlib.h"
#include "string.h"
#include <stdbool.h>

extern struct vfs_impl initrd_impl;

struct {
	const char *name;
	struct vfs_impl *impl;
} fs_list[] = {
	{"initrd", &initrd_impl},
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

struct inode *vfs_get_inode(const char *path, int *err) {
	struct path parsed = parse_path(path);
	struct inode *node = root;
	for (int i = 0; parsed.components[i].name; i++) {
		if ((node->mode & S_IFMT) != S_IFDIR) {
			*err = ENOTDIR;
			return NULL;
		}
		node->fs->impl->getdents(node);
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

int vfs_mount(const char *source, const char *target, const char *fs, uint32_t flags) {
	if (!(flags & MS_RDONLY)) {
		return -ENOTSUP;
	}

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
	if (flags) {
		*err = ENOTSUP;
		return NULL;
	}
	struct inode *node = vfs_get_inode(path, err);
	if (!node) {
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
	int res = f->inode->fs->impl->pread(f->inode, buf, count, f->pos);
	if (res > 0) {
		f->pos += res;
	}
	return res;
}

