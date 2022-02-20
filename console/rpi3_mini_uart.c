#include "rpi3_mini_uart.h"
#include "console.h"
#include "mmio.h"

extern void __gppud_delay();

static void enable_gpio_mini_uart() {
	uint32_t sel1 = *GPFSEL1;
	sel1 &= ~((FSELMASK << FSEL14_OFFSET) | (FSELMASK << FSEL15_OFFSET));
	sel1 |= (ALT5 << FSEL14_OFFSET) | (ALT5 << FSEL15_OFFSET);
	*GPFSEL1 = sel1;

	*GPPUD = 0;
	__gppud_delay();
	*GPPUDCLK0 = (1 << 14) | (1 << 15);
	__gppud_delay();
	*GPPUDCLK0 = 0;
}

static void rpi3_mini_uart_putc(uint8_t c) {
	while (!(*AUX_MU_LSR_REG & AUX_MU_LSR_TX_READY));
	*AUX_MU_IO_REG = c;
}

static uint8_t rpi3_mini_uart_getc() {
	while (!(*AUX_MU_LSR_REG & AUX_MU_LSR_RX_READY));
	return *AUX_MU_IO_REG;
}

static const struct console rpi3_mini_uart_con = {
	.putc = rpi3_mini_uart_putc,
	.getc = rpi3_mini_uart_getc,
};

const struct console *rpi3_mini_uart_setup() {
	*AUXENB = AUXENB_MINIUART;
	*AUX_MU_CNTL_REG = 0;
	*AUX_MU_IER_REG = 0; // TODO: interrupt support
	*AUX_MU_LCR_REG = AUX_MU_LCR_8BIT;
	*AUX_MU_MCR_REG = 0;
	*AUX_MU_BAUD = 270; // 250M / (8 x (AUX_MU_BAUD + 1)), targeting 115200

	enable_gpio_mini_uart();

	*AUX_MU_IIR_REG = AUX_MU_IIR_RFIFO_CLEAR | AUX_MU_IIR_WFIFO_CLEAR;
	*AUX_MU_CNTL_REG = AUX_MU_CNTL_RX_ENABLE | AUX_MU_CNTL_TX_ENABLE;

	return &rpi3_mini_uart_con;
}
