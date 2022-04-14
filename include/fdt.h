#ifndef FDT_H
#define FDT_H

#include <stdbool.h>
#include <stdint.h>

#define FDT_MAGIC	0xd00dfeed

#define FDT_BEGIN_NODE	0x1
#define FDT_END_NODE	0x2
#define FDT_PROP	0x3
#define FDT_NOP	0x4
#define FDT_END	0x9

#define FDT_NOT_PRESENT	((uint8_t *) UINTPTR_MAX)

extern uint8_t *fdt;
extern uint8_t *fdt_dt_struct;
extern uint8_t *fdt_dt_strings;
extern char fdt_full_path[1024];

struct fdt_header {
	uint32_t magic;
	uint32_t totalsize;
	uint32_t off_dt_struct;
	uint32_t off_dt_strings;
	uint32_t off_mem_rsvmap;
	uint32_t version;
	uint32_t last_comp_version;
	uint32_t boot_cpuid_phys;
	uint32_t size_dt_strings;
	uint32_t size_dt_struct;
};

struct fdt_reserve_entry {
	uint64_t address;
	uint64_t size;
};

struct fdt_prop {
	uint32_t len;
	uint32_t nameoff;
};

void fdt_init();
void *fdt_traverse(bool (*callback)(uint32_t *token));
void *fdt_get_prop(uint32_t *token, const char *name, uint32_t *len);
uint32_t fdt_prop_uint32(void *prop);
uint64_t fdt_prop_uint64(void *prop);

#endif
