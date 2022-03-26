#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#define ENABLE_INTERRUPTS() __asm__("msr DAIFClr, 0xf")
#define DISABLE_INTERRUPTS() __asm__("msr DAIFSet, 0xf")

#endif
