#include "fdt.h"
#include "init.h"
#include "kio.h"
#include "bcm2835_mini_uart.h"
#include "bcm2835_mailbox.h"
#include "bcm2836_ic.h"
#include "page.h"
#include "string.h"
#include "timer.h"

uint8_t *initrd_start = (uint8_t *) 0x8000000;

static void *initrd_end;

extern void arm_exceptions();

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
	enable_core0_cntp_irq();
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
