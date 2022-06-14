#include "errno.h"
#include "endianness.h"
#include "kio.h"
#include "vfs.h"
#include "stdlib.h"
#include "string.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define FAT_BPB_BYTES_PER_SECTOR_OFFSET	0x0b
#define FAT_BPB_SECTORS_PER_CLUSTER_OFFSET	0x0d
#define FAT_BPB_RESERVED_SECTORS_OFFSET	0x0e
#define FAT_BPB_FAT_COUNT_OFFSET	0x10

#define FAT32_EBR_FAT_SIZE_OFFSET	0x24
#define FAT32_EBR_ROOT_OFFSET	0x2c
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

struct fat32_state {
	int bytes_per_sector;
	int sectors_per_cluster;
	int fat_offset;
	int fat_count;
	int fat_size;
	size_t data_offset;
	struct fd *dev;
};

struct fat32_entry {
	char name[8], ext[3];
	uint8_t attr;
	uint8_t res;
	uint8_t ctime_1;
	uint16_t ctime_2, ctime_3;
	uint16_t atime;
	uint16_t cluster_high;
	uint16_t mtime_1, mtime_2;
	uint16_t cluster_low;
	uint32_t sz;
};

enum {
	FAT_ATTR_READ_ONLY	= 0x1,
	FAT_ATTR_DIRECTORY	= 0x10,
	FAT_ATTR_LFN	= 0xf,
};

static uint32_t attr_to_mode(uint8_t attr) {
	bool is_dir = attr & FAT_ATTR_DIRECTORY;
	bool is_readonly = attr & FAT_ATTR_READ_ONLY;
	if (is_dir) {
		return is_readonly ? (S_IFDIR | 0444) : (S_IFDIR | 0644);
	} else {
		return is_readonly ? (S_IFREG | 0555) : (S_IFREG | 0755);
	}
}

static void kput32x(uint32_t val) {
	char hexbuf[9];
	hexbuf[8] = '\0';
	for (int i = 0; i < 8; i++) {
		int dig = val & 0xf;
		hexbuf[7 - i] = (dig > 9 ? 'a' - 10 : '0') + dig;
		val >>= 4;
	}
	kputs(hexbuf);
}

static int next_cluster(uint32_t current, struct fat32_state *state) {
	int entries_per_sector = state->bytes_per_sector / 4;
	int offset = (state->fat_offset + current / entries_per_sector) * state->bytes_per_sector;
	uint32_t entries[entries_per_sector];
	int res = vfs_pread(state->dev, entries, state->bytes_per_sector, offset);
	if (res < 0) {
		return res;
	}
	int val = entries[current % entries_per_sector] & 0x0FFFFFFF;
	if (val >= 0x0FFFFFF7) {
		return 0;
	}
	return val;
}

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
	target->size = 0;
	target->entries = NULL;
	target->fs = malloc(sizeof(struct fs));
	target->fs->flags = flags;
	target->fs->impl = &fat32_impl;
	struct fat32_state *state = malloc(sizeof (struct fat32_state));
	target->fs->data = state;
	state->dev = dev;
	state->bytes_per_sector = ((uint16_t)br[FAT_BPB_BYTES_PER_SECTOR_OFFSET + 1] << 8) |
		br[FAT_BPB_BYTES_PER_SECTOR_OFFSET];
	if (state->bytes_per_sector % sizeof(struct fat32_entry)) {
		return -ENOTSUP;
	}
	state->sectors_per_cluster = br[FAT_BPB_SECTORS_PER_CLUSTER_OFFSET];
	state->fat_offset = FROM_LE16(*(uint16_t *)&br[FAT_BPB_RESERVED_SECTORS_OFFSET]);
	state->fat_count = br[FAT_BPB_FAT_COUNT_OFFSET];
	state->fat_size = FROM_LE32(*(uint32_t *)&br[FAT32_EBR_FAT_SIZE_OFFSET]);
	state->data_offset = state->fat_offset + state->fat_size * state->fat_count - 2;
	target->data = (void *)((uint64_t) FROM_LE32(*((uint32_t *)&br[FAT32_EBR_ROOT_OFFSET])));
	return 0;
}

static int fat32_getdents(struct inode *inode) {
	struct fat32_state *state = inode->fs->data;
	struct dentry **next = &inode->entries;
	int count = 0;
	uint64_t cluster = (uint64_t)inode->data;
	do {
		int offset = (cluster * state->sectors_per_cluster +
				state->data_offset) * state->bytes_per_sector;
		for (int i = 0; i < state->sectors_per_cluster; i++) {
			int l = state->bytes_per_sector / sizeof(struct fat32_entry);
			struct fat32_entry entries[l];
			int res = vfs_pread(state->dev, entries, state->bytes_per_sector, offset + i * state->bytes_per_sector);
			if (res < 0) {
				return res;
			}
			for (int j = 0; j < l; j++) {
				if (!entries[j].name[0]) {
					*next = NULL;
					inode->size = count;
					return count;
				}
				if (entries[j].name[0] == 0xe5) {
					continue;
				}
				if (entries[j].attr == FAT_ATTR_LFN) {
					continue;
				}
				if (!strncmp(entries[j].name, ".           ", 11) ||
						!strncmp(entries[j].name, "..          ", 11)) {
					continue;
				}
				*next = malloc(sizeof(struct dentry));
				(*next)->name = malloc(sizeof(char) * 13);
				strncpy((*next)->name, entries[j].name, 8);
				(*next)->name[8] = '\0';
				int idx = 7;
				while (idx && (*next)->name[idx] == ' ') {
					(*next)->name[idx] = '\0';
					idx--;
				}
				if (entries[j].ext[0]) {
					idx++;
					(*next)->name[idx] = '.';
					idx++;
					strncpy(&(*next)->name[idx], entries[j].ext, 3);
					idx += 3;
					(*next)->name[idx] = '\0';
					idx--;
					while (idx && (*next)->name[idx] == ' ') {
						(*next)->name[idx] = '\0';
						idx--;
					}
				}
				(*next)->len = strlen((*next)->name);
				(*next)->inode = malloc(sizeof(struct inode));
				(*next)->inode->fs = inode->fs;
				(*next)->inode->data = (void *)(((uint64_t)FROM_LE16(entries[j].cluster_high) << 16) |
					FROM_LE16(entries[j].cluster_low));
				(*next)->inode->size = FROM_LE32(entries[j].sz);
				(*next)->inode->mode = attr_to_mode(entries[j].attr);
				(*next)->inode->entries = NULL;
				next = &(*next)->next;
				count++;
			}
		}
	} while ((cluster = next_cluster(cluster, state)));
	*next = NULL;
	inode->size = count;
	return count;
}

static int64_t fat32_pread(struct inode *inode, void *buf, size_t count, size_t offset) {
	struct fat32_state *state = inode->fs->data;
	int cluster_now = 0;
	uint64_t cluster = (uint64_t)inode->data;
	size_t cluster_size = state->bytes_per_sector * state->sectors_per_cluster;
	int read = 0;
	uint8_t *ubuf = buf;
	do {
		if (!count) {
			break;
		}
		int cluster_end = cluster_now + cluster_size;
		if (offset < cluster_end) {
			int max_available = cluster_end - offset;
			int local_count = count < max_available ? count : max_available;
			int left = local_count;

			uint8_t sector[state->bytes_per_sector];
			size_t cluster_offset = (cluster * state->sectors_per_cluster +
				state->data_offset) * state->bytes_per_sector;
			int sector_idx = (offset - cluster_now) / state->bytes_per_sector;
			int in_sector_offset = (offset - cluster_now) % state->bytes_per_sector;
			int res = vfs_pread(state->dev, sector, state->bytes_per_sector,
				cluster_offset + sector_idx * state->bytes_per_sector);
			if (res < 0) {
				return -res;
			}
			while (left && in_sector_offset < state->bytes_per_sector) {
				ubuf[read++] = sector[in_sector_offset++];
				left--;
			}
			for (int i = sector_idx + 1; left && i < state->bytes_per_sector; i++) {
				int res = vfs_pread(state->dev, sector, state->bytes_per_sector,
					cluster_offset + i * state->bytes_per_sector);
				if (res < 0) {
					return -res;
				}
				int j = 0;
				while (left && j < state->bytes_per_sector) {
					ubuf[read++] = sector[j++];
					left--;
				}
			}

			offset += local_count;
			count -= local_count;
		}
		cluster_now = cluster_end;
	} while ((cluster = next_cluster(cluster, state)));
	return read;
}

