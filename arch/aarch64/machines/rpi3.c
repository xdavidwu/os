#include "init.h"
#include "kio.h"
#include "rpi3_mini_uart.h"

void machine_init() {
	kconsole = rpi3_mini_uart_setup();
	main();
}
