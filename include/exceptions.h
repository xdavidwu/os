#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

extern int interrupt_ref;
extern int in_exception;

#define ENABLE_INTERRUPTS() do {\
	if (!in_exception) {\
		if (!interrupt_ref) {\
			__asm__("msr DAIFClr, 0xf");\
		}\
		interrupt_ref++;\
	}\
} while (0)

// TODO detect disable when not enabled
#define DISABLE_INTERRUPTS() do {\
	if (!in_exception) {\
		interrupt_ref--;\
		if (!interrupt_ref) {\
			__asm__("msr DAIFSet, 0xf");\
		}\
	}\
} while (0)

#endif
