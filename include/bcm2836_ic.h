#ifndef BCM2836_IC_H
#define BCM2836_IC_H

#include <stdint.h>

#define BCM2836_IC_BASE	0x40000000

#define CORE0_TIMER_IRQCNTL	((uint32_t *) (BCM2836_IC_BASE + 0x40))

enum {
	CNTP_IRQ = 0x2,
};

#define CORE0_IRQ_SOURCE	((uint32_t *) (BCM2836_IC_BASE + 0x60))

enum {
	IRQ_SRC_CNTP = 0x2,
};

void enable_core0_cntp_irq();

#endif
