#ifndef CON_H
#define CON_H

#include <stdint.h>

// TODO generic interface
void rpi3_mini_uart_setup();
void rpi3_mini_uart_putc(uint8_t c);

#endif
