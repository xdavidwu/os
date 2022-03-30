#include "bcm2836_ic.h"
#include "exceptions.h"
#include "kio.h"

int interrupt_ref = 0;
int in_exception = 0;

static void kput64x(uint64_t val) {
	char hexbuf[17];
	hexbuf[16] = '\0';
	for (int i = 0; i < 16; i++) {
		int dig = val & 0xf;
		hexbuf[15 - i] = (dig > 9 ? 'a' - 10 : '0') + dig;
		val >>= 4;
	}
	kputs(hexbuf);
}

void handle_unimplemented() {
	in_exception = 1;
	uint64_t spsr_el1, elr_el1, esr_el1;
	__asm__ ("mrs %0, spsr_el1" : "=r" (spsr_el1));
	__asm__ ("mrs %0, elr_el1" : "=r" (elr_el1));
	__asm__ ("mrs %0, esr_el1" : "=r" (esr_el1));
	kputs("Unimplemeted exception:\nspsr_el1:\t");
	kput64x(spsr_el1);
	kputs("\nelr_el1:\t");
	kput64x(elr_el1);
	kputs("\nesr_el1:\t");
	kput64x(esr_el1);
	kputc('\n');
	in_exception = 0;
}

#define TASKS_MAX 32
struct tasks {
	void (*func)(void *);
	void *data;
	int prio;
};

struct tasks *tasks_local_p;

static int tasks_local_idx;

void register_task(void (*func)(void *), void *data, int prio) {
	tasks_local_p[tasks_local_idx].func = func;
	tasks_local_p[tasks_local_idx].data = data;
	tasks_local_p[tasks_local_idx].prio = prio;
	tasks_local_idx++;
}

void handle_irq() {
	struct tasks tasks_local[TASKS_MAX];
	tasks_local_p = tasks_local;
	in_exception = 1;
	tasks_local_idx = 0;
	bcm2836_handle_irq();

	int tasks_local_max = tasks_local_idx;
	in_exception = 0;
	__asm__ ("msr DAIFClr, 0xf");

	for (int i = 0; i < tasks_local_max; i++) {
		tasks_local[i].func(tasks_local[i].data);
	}

	__asm__ ("msr DAIFSet, 0xf");
}
