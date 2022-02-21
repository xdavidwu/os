#include "init.h"
#include "kio.h"
#include "bcm2835_mini_uart.h"

void machine_init() {
	struct console kcon;
	cinit(&kcon, bcm2835_mini_uart_setup());
	kconsole = &kcon;
	main();
}
