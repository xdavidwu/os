#include "bcm2835_mini_uart.h"
#include "bcm2835_mmio.h"
#include "bcm2835_ic.h"
#include "console.h"
#include "errno.h"

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

static void bcm2835_mini_uart_putc(uint8_t c) {
	while (!(*AUX_MU_LSR_REG & AUX_MU_LSR_TX_READY));
	*AUX_MU_IO_REG = c;
}

static uint8_t bcm2835_mini_uart_getc() {
	while (!(*AUX_MU_LSR_REG & AUX_MU_LSR_RX_READY));
	return *AUX_MU_IO_REG;
}

static int bcm2835_mini_uart_putc_nonblock(uint8_t c) {
	if (!(*AUX_MU_LSR_REG & AUX_MU_LSR_TX_READY)) {
		return -EWOULDBLOCK;
	}
	*AUX_MU_IO_REG = c;
	return SUCCESS;
}

static int bcm2835_mini_uart_getc_nonblock() {
	if (!(*AUX_MU_LSR_REG & AUX_MU_LSR_RX_READY)) {
		return -EWOULDBLOCK;
	}
	return *AUX_MU_IO_REG;
}

static void bcm2835_mini_uart_tx_interrupt(bool enable) {
	if (enable) {
		*AUX_MU_IER_REG |= AUX_MU_CNTL_TX_ENABLE;
	} else {
		*AUX_MU_IER_REG &= ~AUX_MU_CNTL_TX_ENABLE;
	}
}

static void bcm2835_mini_uart_rx_interrupt(bool enable) {
	if (enable) {
		*AUX_MU_IER_REG |= AUX_MU_CNTL_RX_ENABLE;
	} else {
		*AUX_MU_IER_REG &= ~AUX_MU_CNTL_RX_ENABLE;
	}
}

static const struct console_impl bcm2835_mini_uart_con = {
	.putc = bcm2835_mini_uart_putc,
	.getc = bcm2835_mini_uart_getc,
	.putc_nonblock = bcm2835_mini_uart_putc_nonblock,
	.getc_nonblock = bcm2835_mini_uart_getc_nonblock,
	.set_tx_interrupt = bcm2835_mini_uart_tx_interrupt,
	.set_rx_interrupt = bcm2835_mini_uart_rx_interrupt,
};

void bcm2835_mini_uart_setup(struct console *con) {
	*AUXENB = AUXENB_MINIUART;
	*AUX_MU_CNTL_REG = 0;
	*AUX_MU_IER_REG = 0;
	*AUX_MU_LCR_REG = AUX_MU_LCR_8BIT;
	*AUX_MU_MCR_REG = 0;
	*AUX_MU_BAUD = 270; // 250M / (8 x (AUX_MU_BAUD + 1)), targeting 115200
	*IC_IRQ_ENBALE1 = 1 << 29;

	enable_gpio_mini_uart();

	*AUX_MU_IIR_REG = AUX_MU_IIR_RFIFO_CLEAR | AUX_MU_IIR_WFIFO_CLEAR;
	*AUX_MU_CNTL_REG = AUX_MU_CNTL_RX_ENABLE | AUX_MU_CNTL_TX_ENABLE;

	con->impl = &bcm2835_mini_uart_con;
	cinit(con);
	bcm2835_armctrl_set_irq_handler(29, (void (*)(void *))cflush_nonblock, con);
}
