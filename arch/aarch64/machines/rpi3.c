#include "init.h"
#include "kio.h"
#include "bcm2835_mini_uart.h"
#include "bcm2835_mailbox.h"

void machine_init() {
	struct console kcon;
	cinit(&kcon, bcm2835_mini_uart_setup());
	kconsole = &kcon;
	bcm2835_mbox_print_info();
	main();
}
