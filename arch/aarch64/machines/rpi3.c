#include "init.h"
#include "kio.h"
#include "bcm2835_mini_uart.h"
#include "bcm2835_mailbox.h"
#include "bcm2836_ic.h"
#include "timer.h"

uint8_t *initrd_start = (uint8_t *) 0x8000000;

extern void arm_exceptions();

void machine_init() {
	arm_timer();
	struct console kcon;
	cinit(&kcon, bcm2835_mini_uart_setup());
	kconsole = &kcon;
	bcm2835_mbox_print_info();
	arm_exceptions();
	enable_core0_cntp_irq();
	main();
}
