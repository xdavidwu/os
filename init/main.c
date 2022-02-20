#include "con.h"

int main() {
	rpi3_mini_uart_setup();
	rpi3_mini_uart_putc('H');
	rpi3_mini_uart_putc('e');
	rpi3_mini_uart_putc('l');
	rpi3_mini_uart_putc('l');
	rpi3_mini_uart_putc('o');
	rpi3_mini_uart_putc(' ');
	rpi3_mini_uart_putc('W');
	rpi3_mini_uart_putc('o');
	rpi3_mini_uart_putc('r');
	rpi3_mini_uart_putc('l');
	rpi3_mini_uart_putc('d');
	rpi3_mini_uart_putc('!');
	while (1) {
	}
}
