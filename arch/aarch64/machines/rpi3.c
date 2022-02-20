#include "init.h"
#include "kio.h"
#include "bcm2835_mini_uart.h"

void machine_init() {
	kconsole = bcm2835_mini_uart_setup();
	main();
}
