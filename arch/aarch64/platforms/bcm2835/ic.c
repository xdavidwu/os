#include "bcm2835_mmio.h"
#include "bcm2835_ic.h"

static struct {
	void (*func)(void *);
	void *data;
} armctl_irq_table[64];

void bcm2835_armctrl_handle_irq() {
	uint32_t src_base = *IC_IRQ_PENDING_BASIC,
		src1 = *IC_IRQ_PENDING_1, src2 = *IC_IRQ_PENDING_2;
	if ((src_base & IC_IRQ_PENDING_1_SET) == IC_IRQ_PENDING_1_SET) {
		int bit = 0;
		while (src1) {
			if (src1 & 0x1) {
				if (armctl_irq_table[bit].func) {
					armctl_irq_table[bit].func(armctl_irq_table[bit].data);
				}
			}
			bit++;
			src1 >>= 1;
		}
	} else if ((src_base & IC_IRQ_PENDING_2_SET) == IC_IRQ_PENDING_2_SET) {
		int bit = 32;
		while (src2) {
			if (src2 & 0x1) {
				if (armctl_irq_table[bit].func) {
					armctl_irq_table[bit].func(armctl_irq_table[bit].data);
				}
			}
			bit++;
			src2 >>= 1;
		}
	}
}

void bcm2835_armctrl_set_irq_handler(int bits, void (*func)(void *), void *data) {
	armctl_irq_table[bits].func = func;
	armctl_irq_table[bits].data = data;
}
