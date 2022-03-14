#include "fdt.h"
#include "init.h"
#include "kshell.h"
#include "string.h"

static bool initrd_set_addr(uint32_t *token) {
	static bool handled = false;
	if (!strcmp(fdt_full_path, "/chosen")) {
		handled = true;
		uint32_t len;
		void *prop = fdt_get_prop(token, "linux,initrd-start", &len);
		if (prop) {
			initrd_start = (uint8_t *) (len == 4 ?
				fdt_prop_uint32(prop) : fdt_prop_uint64(prop));
		}
		return true;
	}
	return handled;
}

void main() {
	fdt_init();
	fdt_traverse(initrd_set_addr);
	kshell();
}
