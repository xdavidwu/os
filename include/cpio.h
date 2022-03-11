#ifndef CPIO_H
#define CPIO_H

#include <stdbool.h>
#include <stdint.h>

#define CPIO_NEWC_MAGIC	"070701"
#define CPIO_END_NAME	"TRAILER!!!"

struct cpio_newc_header {
	char c_magic[6];
	char c_ino[8];
	char c_mode[8];
	char c_uid[8];
	char c_gid[8];
	char c_nlink[8];
	char c_mtime[8];
	char c_filesize[8];
	char c_devmajor[8];
	char c_devminor[8];
	char c_rdevmajor[8];
	char c_rdevminor[8];
	char c_namesize[8]; // including \0
	char c_check[8];
};

uint32_t cpio_get_uint32(const char *ptr);

char *cpio_get_name(uint8_t *entry);

uint8_t *cpio_get_file(uint8_t *entry, uint32_t namesz);

bool cpio_is_end(uint8_t *entry);

uint8_t *cpio_next_entry(uint8_t *entry, uint32_t namesz, uint32_t filesz);

#endif
