#include "endianness.h"
#include "fdt.h"
#include "string.h"
#include <stdbool.h>
#include <stdint.h>

uint8_t *fdt;
uint8_t *fdt_dt_struct;
uint8_t *fdt_dt_strings;
char fdt_full_path[1024];

void fdt_init() {
	struct fdt_header *fdt_header = (struct fdt_header *) fdt;
	if (FROM_BE32(fdt_header->magic) != FDT_MAGIC) {
		fdt = FDT_NOT_PRESENT;
		return;
	}
	fdt_dt_struct = fdt + FROM_BE32(fdt_header->off_dt_struct);
	fdt_dt_strings = fdt + FROM_BE32(fdt_header->off_dt_strings);
}

static uint32_t *fdt_recur(uint32_t *token, int path_index,
		bool (*callback)(uint32_t *token)) {
	token++;
	int len = 0;
	if (!path_index) {
		strcpy(fdt_full_path, "/");
		path_index = 1;
	} else {
		if (path_index != 1) {
			fdt_full_path[path_index++] = '/';
		}
		strcpy(&fdt_full_path[path_index], (char *) token);
		len = strlen((char *) token);
		path_index += len;
	}
	token += (len + 1 + 3) / 4;
	bool handled = callback ? callback(token) : true;
	while (1) {
		struct fdt_prop *prop;
		switch (FROM_BE32(*token)) {
		case FDT_NOP:
			token++;
			break;
		case FDT_PROP:
			prop = (struct fdt_prop *) (++token);
			token += sizeof(struct fdt_prop) / 4;
			if (FROM_BE32(prop->len)) {
				token += (FROM_BE32(prop->len) + 3) / 4;
			}
			break;
		case FDT_BEGIN_NODE:
			token = fdt_recur(token, path_index,
				handled ? NULL : callback);
			break;
		case FDT_END_NODE:
			return ++token;
		default:
			return token;
		}
	}
}

void *fdt_get_prop(uint32_t *token, const char *name, uint32_t *len) {
	while (1) {
		struct fdt_prop *prop;
		switch (FROM_BE32(*token)) {
		case FDT_NOP:
			token++;
			break;
		case FDT_PROP:
			prop = (struct fdt_prop *) (++token);
			token += sizeof(struct fdt_prop) / 4;
			if (!strcmp((char *) fdt_dt_strings +
					FROM_BE32(prop->nameoff), name)) {
				if (len) {
					*len = FROM_BE32(prop->len);
				}
				return token;
			}
			if (FROM_BE32(prop->len)) {
				token += (FROM_BE32(prop->len) + 3) / 4;
			}
			break;
		case FDT_BEGIN_NODE:
			return NULL;
			break;
		case FDT_END_NODE:
			return NULL;
		default:
			return NULL;
		}
	}
}

uint32_t fdt_prop_uint32(void *prop) {
	return FROM_BE32(*(uint32_t *) prop);
}

uint64_t fdt_prop_uint64(void *prop) {
	uint32_t *prop32 = prop;
	uint64_t first = FROM_BE32(*prop32);
	return (first << 32) | FROM_BE32(*++prop32);
}

void *fdt_traverse(bool (*callback)(uint32_t *token)) {
	if (fdt == FDT_NOT_PRESENT) {
		return NULL;
	}
	return fdt_recur((uint32_t *) fdt_dt_struct, 0, callback);
}
