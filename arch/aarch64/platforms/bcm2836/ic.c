#include "bcm2836_ic.h"
#include "bcm2835_ic.h"
#include "exceptions.h"
#include "kio.h"
#include "kthread.h"
#include "timer.h"

void enable_core0_cnt_irq() {
	*CORE0_TIMER_IRQCNTL = CNTP_IRQ | CNTV_IRQ;
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


void bcm2836_handle_irq() {
	uint32_t src = *CORE0_IRQ_SOURCE;
	if ((src & IRQ_SRC_CNTP) == IRQ_SRC_CNTP) {
		handle_timer();
	} else if ((src & IRQ_SRC_CNTV) == IRQ_SRC_CNTV) {
		uint64_t freq;
		asm("mrs %0, cntfrq_el0" : "=r" (freq));
		freq >>= 5;
		asm("msr cntv_tval_el0, %0" : : "r" (freq));
		register_task(kthread_yield, NULL, 0);
	} else if ((src & IRQ_SRC_GPU) == IRQ_SRC_GPU) {
		bcm2835_armctrl_handle_irq();
	} else {
		kputs("Unrecognized irq: ");
		kpu32x(src);
		kputc('\n');
	}
}
