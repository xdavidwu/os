#include "init.h"
#include "kio.h"
#include "bcm2835_mini_uart.h"
#include "bcm2835_mailbox.h"
#include "bcm2836_ic.h"
#include "timer.h"

uint8_t *initrd_start = (uint8_t *) 0x8000000;

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

void machine_init() {
	register_timer(2, timer_act, NULL, -20);
	struct console kcon;
	bcm2835_mini_uart_setup(&kcon);
	kconsole = &kcon;
	arm_exceptions();
	enable_core0_cntp_irq();
	bcm2835_mbox_print_info();
	main();
}
