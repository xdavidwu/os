#include "bcm2836_ic.h"
#include "exceptions.h"
#include "kio.h"
#include "timer.h"

void enable_core0_cntp_irq() {
	*CORE0_TIMER_IRQCNTL = CNTP_IRQ;
}

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

void handle_irq() {
	static uint32_t uptime = 0;
	uint32_t src = *CORE0_IRQ_SOURCE;
	switch (src) {
	default:
		uptime += 2;
		kputs("Uptime: ");
		kpu32x(uptime);
		kputc('\n');
		rearm_timer();
		break;
	}
}
