#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

extern int interrupt_ref;
extern int in_exception;

#define DISABLE_INTERRUPTS() do {\
	if (!in_exception) {\
		if (!interrupt_ref) {\
			__asm__("msr DAIFSet, 0xf\nisb");\
		}\
		interrupt_ref++;\
	}\
} while (0)

// TODO detect enable when not disabled
#define ENABLE_INTERRUPTS() do {\
	if (!in_exception) {\
		interrupt_ref--;\
		if (!interrupt_ref) {\
			__asm__("msr DAIFClr, 0xf");\
		}\
	}\
} while (0)

#define PRIO_MAX	-20
#define PRIO_MIN	19

void register_task(void (*func)(void *), void *data, int prio);

#endif
