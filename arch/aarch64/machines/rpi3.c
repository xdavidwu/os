#include "aarch64/vmem.h"
#include "fdt.h"
#include "init.h"
#include "kio.h"
#include "bcm2835_mini_uart.h"
#include "bcm2835_mailbox.h"
#include "bcm2836_ic.h"
#include "page.h"
#include "string.h"
#include "timer.h"
#include <stdint.h>

uint8_t *initrd_start = (uint8_t *) 0x8000000;

static void *initrd_end;

extern void arm_exceptions();

#define PMD_ENTRY_16M(base, attr) \
	(base + 0) | (attr), \
	(base + 0x200000) | (attr), \
	(base + 0x400000) | (attr), \
	(base + 0x600000) | (attr), \
	(base + 0x800000) | (attr), \
	(base + 0xa00000) | (attr), \
	(base + 0xc00000) | (attr), \
	(base + 0xe00000) | (attr)

#define PMD_ENTRY_64M(base, attr) \
	PMD_ENTRY_16M((base + 0), attr), \
	PMD_ENTRY_16M((base + 0x1000000), attr), \
	PMD_ENTRY_16M((base + 0x2000000), attr), \
	PMD_ENTRY_16M((base + 0x3000000), attr)

uint64_t __attribute__((aligned(4096))) pmd[] = {
	PMD_ENTRY_64M(0, PD_ACCESS | (MAIR_IDX_NORMAL_NOCACHE << 2) | PD_BLOCK),
	PMD_ENTRY_64M(0x4000000, PD_ACCESS | (MAIR_IDX_NORMAL_NOCACHE << 2) | PD_BLOCK),
	PMD_ENTRY_64M(0x8000000, PD_ACCESS | (MAIR_IDX_NORMAL_NOCACHE << 2) | PD_BLOCK),
	PMD_ENTRY_64M(0xc000000, PD_ACCESS | (MAIR_IDX_NORMAL_NOCACHE << 2) | PD_BLOCK),
	PMD_ENTRY_64M(0x10000000, PD_ACCESS | (MAIR_IDX_NORMAL_NOCACHE << 2) | PD_BLOCK),
	PMD_ENTRY_64M(0x14000000, PD_ACCESS | (MAIR_IDX_NORMAL_NOCACHE << 2) | PD_BLOCK),
	PMD_ENTRY_64M(0x18000000, PD_ACCESS | (MAIR_IDX_NORMAL_NOCACHE << 2) | PD_BLOCK),
	PMD_ENTRY_64M(0x1c000000, PD_ACCESS | (MAIR_IDX_NORMAL_NOCACHE << 2) | PD_BLOCK),
	PMD_ENTRY_64M(0x20000000, PD_ACCESS | (MAIR_IDX_NORMAL_NOCACHE << 2) | PD_BLOCK),
	PMD_ENTRY_64M(0x24000000, PD_ACCESS | (MAIR_IDX_NORMAL_NOCACHE << 2) | PD_BLOCK),
	PMD_ENTRY_64M(0x28000000, PD_ACCESS | (MAIR_IDX_NORMAL_NOCACHE << 2) | PD_BLOCK),
	PMD_ENTRY_64M(0x2c000000, PD_ACCESS | (MAIR_IDX_NORMAL_NOCACHE << 2) | PD_BLOCK),
	PMD_ENTRY_64M(0x30000000, PD_ACCESS | (MAIR_IDX_NORMAL_NOCACHE << 2) | PD_BLOCK),
	PMD_ENTRY_64M(0x34000000, PD_ACCESS | (MAIR_IDX_NORMAL_NOCACHE << 2) | PD_BLOCK),
	PMD_ENTRY_64M(0x38000000, PD_ACCESS | (MAIR_IDX_NORMAL_NOCACHE << 2) | PD_BLOCK),
	PMD_ENTRY_64M(0x3c000000, PD_ACCESS | (MAIR_IDX_DEVICE_nGnRnE << 2) | PD_BLOCK),
};

uint64_t __attribute__((aligned(4096))) pud[] = {
	(uint64_t)pmd - HIGH_MEM_OFFSET + PD_TABLE,
	0x40000000 | PD_ACCESS | (MAIR_IDX_DEVICE_nGnRnE << 2) | PD_BLOCK,
};

// | PD_TABLE is not constant but + PD_TABLE is, on GCC
// 
// https://stackoverflow.com/questions/58496061/why-bitwise-or-doesnt-result-in-a-constant-expression-but-addition-does
//
// sounds like any operation after a cast is constant or not is not guranteed by spec
//
// FIXME: GCC-ism?
uint64_t __attribute__((aligned(4096))) pgd[] = {
	(uint64_t)pud - HIGH_MEM_OFFSET + PD_TABLE,
};

static void kpu32x(uint32_t val) {
	char hexbuf[9];
	hexbuf[8] = '\0';
	for (int i = 0; i < 8; i++) {
		int dig = val & 0xf;
		hexbuf[7 - i] = (dig > 9 ? 'a' - 10 : '0') + dig;
		val >>= 4;
	}
	kputs(hexbuf);
}

static void timer_act() {
	static unsigned int uptime = 0;
	register_timer(2, timer_act, NULL, -20);
	uptime += 2;
	kputs("Uptime: ");
	kpu32x(uptime);
	kputs("\n");
}

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
		prop = fdt_get_prop(token, "linux,initrd-end", &len);
		if (prop) {
			initrd_end = (void *) (len == 4 ?
				fdt_prop_uint32(prop) : fdt_prop_uint64(prop));
		}
		return true;
	}
	return handled;
}

void machine_init() {
	//register_timer(2, timer_act, NULL, -20);
	arm_exceptions();
	struct console kcon;
	bcm2835_mini_uart_setup(&kcon);
	kconsole = &kcon;
	enable_core0_cnt_irq();
	bcm2835_mbox_print_info();
	fdt_init();
	void *fdt_end = fdt_traverse(initrd_set_addr);
	page_alloc_preinit((void *)0x3c000000);
	mem_reserve(fdt, fdt_end);
	mem_reserve(initrd_start, initrd_end);
	page_alloc_init();
	malloc_init();
	main();
}
