#include "errno.h"
#include "kio.h"
#include "string.h"
#include "vfs.h"
#include "vendor/grasslab/sdhost.h"
#include "stdlib.h"
#include <stddef.h>
#include <stdint.h>

#define SDCARD_DEV	1

#define SDCARD_SECTOR_SIZE	512
#define MBR_PARTITION_ENTRY_OFFSET	0x1be
#define MBR_PARTITION_ENTRY_SIZE	0x10
#define MBR_PARTITION_ENTRY_SYSTEM_ID	0x4
#define MBR_PARTITION_ENTRY_LBA_START	0x8
#define MBR_PARTITION_ENTRY_LBA_COUNT	0xc

struct partition_entry {
	uint32_t offset;
	uint32_t size;
};

static struct partition_entry partitions[4];

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

void sdcard_probe() {
	int err;
	vfs_mknod("/dev/sdcard0", S_IFBLK | 0644, makedev(SDCARD_DEV, 0), &err);
	char *buf = strdup("/dev/sdcard0p0");
	uint8_t sec[SDCARD_SECTOR_SIZE];
	for (int i = 0; i < 4; i++) {
		int this_entry = MBR_PARTITION_ENTRY_OFFSET + (MBR_PARTITION_ENTRY_SIZE * i);
		sec[this_entry + MBR_PARTITION_ENTRY_SYSTEM_ID] = 0;
	}
	readblock(0, sec);
	int valid_index = 1;
	for (int i = 0; i < 4; i++) {
		int this_entry = MBR_PARTITION_ENTRY_OFFSET + (MBR_PARTITION_ENTRY_SIZE * i);
		if (!sec[this_entry + MBR_PARTITION_ENTRY_SYSTEM_ID]) {
			continue;
		}
		partitions[i].offset = (uint32_t)sec[this_entry + MBR_PARTITION_ENTRY_LBA_START + 3] << 24;
		partitions[i].offset |= (uint32_t)sec[this_entry + MBR_PARTITION_ENTRY_LBA_START + 2] << 16;
		partitions[i].offset |= (uint32_t)sec[this_entry + MBR_PARTITION_ENTRY_LBA_START + 1] << 8;
		partitions[i].offset |= sec[this_entry + MBR_PARTITION_ENTRY_LBA_START];
		partitions[i].size = (uint32_t)sec[this_entry + MBR_PARTITION_ENTRY_LBA_COUNT + 3] << 24;
		partitions[i].size |= (uint32_t)sec[this_entry + MBR_PARTITION_ENTRY_LBA_COUNT + 2] << 16;
		partitions[i].size |= (uint32_t)sec[this_entry + MBR_PARTITION_ENTRY_LBA_COUNT + 1] << 8;
		partitions[i].size |= sec[this_entry + MBR_PARTITION_ENTRY_LBA_COUNT];
		kputs("detected parttion: ");
		kput32x(partitions[i].offset);
		kputc(' ');
		kput32x(partitions[i].size);
		kputc('\n');
		buf[13] = '0' + valid_index;
		vfs_mknod(buf, S_IFBLK | 0644, makedev(SDCARD_DEV, valid_index), &err);
		valid_index++;
	}
	free(buf);
}

int64_t sdcard_pread(int minor, void *buf, size_t count, size_t offset) {
	if (minor && !partitions[minor - 1].offset) {
		return -ENODEV;
	}
	if ((offset % SDCARD_SECTOR_SIZE) || (count % SDCARD_SECTOR_SIZE)) {
		return -ENOTSUP;
	}
	int fromlocal = offset / SDCARD_SECTOR_SIZE;
	int from = fromlocal + (minor ? partitions[minor - 1].offset : 0);
	int c = count / SDCARD_SECTOR_SIZE;
	if (minor && fromlocal + c > partitions[minor - 1].size) {
		return -EINVAL;
	}
	uint8_t *ubuf = buf;
	for (int i = 0; i < c; i++) {
		readblock(from + i, ubuf + i * SDCARD_SECTOR_SIZE);
	}
	return count;
}

int64_t sdcard_pwrite(int minor, const void *buf, size_t count, size_t offset) {
	if (minor && !partitions[minor - 1].offset) {
		return -ENODEV;
	}
	if ((offset % SDCARD_SECTOR_SIZE) || (count % SDCARD_SECTOR_SIZE)) {
		return -ENOTSUP;
	}
	int fromlocal = offset / SDCARD_SECTOR_SIZE;
	int from = fromlocal + (minor ? partitions[minor - 1].offset : 0);
	int c = count / SDCARD_SECTOR_SIZE;
	if (minor && fromlocal + c > partitions[minor - 1].size) {
		return -EINVAL;
	}
	const uint8_t *ubuf = buf;
	for (int i = 0; i < c; i++) {
		writeblock(from + i, (void *)(ubuf + i * SDCARD_SECTOR_SIZE));
	}
	return count;
}

