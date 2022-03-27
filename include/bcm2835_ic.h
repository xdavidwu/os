#ifndef BCM2835_IC_H
#define BCM2835_IC_H

void bcm2835_armctrl_handle_irq();

void bcm2835_armctrl_set_irq_handler(int bits, void (*func)(void *), void *data);

#endif
