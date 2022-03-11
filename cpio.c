#include "cpio.h"
#include "string.h"

uint32_t cpio_get_uint32(const char *ptr) {
	uint32_t res = 0;
	for (int i = 0; i < 8; i++) {
		res <<= 4;
		uint8_t val = (*ptr >= 'A') ? (*ptr - 'A' + 10) : (*ptr - '0');
		res |= val;
		ptr++;
	}
	return res;
}

char *cpio_get_name(uint8_t *entry) {
	return (char *)(entry + sizeof(struct cpio_newc_header));
}

uint8_t *cpio_get_file(uint8_t *entry, uint32_t namesz) {
	uint64_t sz = sizeof(struct cpio_newc_header) + namesz;
	return entry + ((sz + 3) >> 2 << 2);
}

bool cpio_is_end(uint8_t *entry) {
	if (strncmp((const char *)entry, CPIO_NEWC_MAGIC, 6)) {
		return true;
	}
	return !strcmp(cpio_get_name(entry), CPIO_END_NAME);
}

uint8_t *cpio_next_entry(uint8_t *entry, uint32_t namesz, uint32_t filesz) {
	uint8_t *addr = cpio_get_file(entry, namesz) + ((filesz + 3) >> 2 << 2);
	return cpio_is_end(addr) ? NULL : addr;
}
