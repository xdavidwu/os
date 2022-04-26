#include "aarch64/registers.h"
#include "bcm2836_ic.h"
#include "exceptions.h"
#include "kio.h"
#include "page.h"
#include "process.h"
#include "syscall.h"
#include <stdbool.h>
#include <stdint.h>

int interrupt_ref = 0;
int in_exception = 0;

extern void exec_signal_handler(void *stack, void (*code)(int), uint64_t *sp, int signal);

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
#define NESTED_IRQ_MAX 16

struct tasks {
	void (*func)(void *);
	void *data;
	int prio;
};

static struct {
	struct tasks *tasks;
	int max_prio;
	int length;
	int pending_index;
} nested_tasks[NESTED_IRQ_MAX];

static int irq_lvl = -1;

void register_task(void (*func)(void *), void *data, int prio) {
	nested_tasks[irq_lvl].tasks[nested_tasks[irq_lvl].length].func = func;
	nested_tasks[irq_lvl].tasks[nested_tasks[irq_lvl].length].data = data;
	nested_tasks[irq_lvl].tasks[nested_tasks[irq_lvl].length].prio = prio;
	nested_tasks[irq_lvl].length++;
}

void handle_irq(bool was_el0) {
	irq_lvl++;
	struct tasks tasks_local[TASKS_MAX + 1]; // + 1 for sen
	nested_tasks[irq_lvl].tasks = tasks_local;
	nested_tasks[irq_lvl].length = 0;
	nested_tasks[irq_lvl].pending_index = 0;
	in_exception = 1;
	bcm2836_handle_irq();

	for (int a = 0; a < nested_tasks[irq_lvl].length; a++) {
		for (int b = a + 1; b < nested_tasks[irq_lvl].length; b++) {
			if (tasks_local[a].prio > tasks_local[b].prio) {
				struct tasks tmp = tasks_local[a];
				tasks_local[a] = tasks_local[b];
				tasks_local[b] = tmp;
			}
		}
	}

	bool touched[irq_lvl];
	for (int b = 0; b < irq_lvl; b++) {
		touched[b] = false;
	}
	// inject low prio to lower nested irqs
	for (int a = nested_tasks[irq_lvl].length; a >= 0; a--) {
		for (int b = 0; b < irq_lvl; b++) {
			if (nested_tasks[b].pending_index >= nested_tasks[b].length) {
				// already empty, ignore
				continue;
			}
			if (nested_tasks[irq_lvl].tasks[a].prio >= nested_tasks[b].max_prio) {
				nested_tasks[b].tasks[nested_tasks[b].length++] = nested_tasks[irq_lvl].tasks[a];
				touched[b] = true;
				// truncate current queue
				nested_tasks[irq_lvl].length = a;
				break;
			}
		}
	}
	// re-sort pending tasks at each level TODO: just do insertion sort?
	for (int b = 0; b < irq_lvl; b++) {
		if (!touched[b]) {
			continue;
		}
		for (int a = nested_tasks[b].pending_index; a < nested_tasks[b].length; a++) {
			for (int c = a + 1; c < nested_tasks[b].length; c++) {
				if (nested_tasks[b].tasks[a].prio > nested_tasks[b].tasks[c].prio) {
					struct tasks tmp = nested_tasks[b].tasks[a];
					nested_tasks[b].tasks[a] = nested_tasks[b].tasks[c];
					nested_tasks[b].tasks[c] = tmp;
				}
			}
		}
	}
	nested_tasks[irq_lvl].max_prio = tasks_local[0].prio;

	in_exception = 0;
	while (true) {
		if (nested_tasks[irq_lvl].pending_index >= nested_tasks[irq_lvl].length) {
			break;
		}
		int taken = nested_tasks[irq_lvl].pending_index++;
		nested_tasks[irq_lvl].max_prio = nested_tasks[irq_lvl].tasks[taken + 1].prio;
		__asm__ ("isb\nmsr DAIFClr, 0xf");
		nested_tasks[irq_lvl].tasks[taken].func(
			nested_tasks[irq_lvl].tasks[taken].data);
		__asm__ ("msr DAIFSet, 0xf\nisb");
	}

	irq_lvl--;

	if (was_el0) {
		struct kthread_states *states;
		__asm__ ("mrs %0, tpidr_el1" : "=r" (states));
		struct process_states *process = states->data;
		if (process->pending_signals && !process->signal_stack) {
			process->signal_stack = page_alloc(1);
			for (int i = 1; i <= SIGNAL_MAX; i++) {
				if ((process->pending_signals & (1 << i)) == (1 << i)) {
					process->pending_signals ^= (1 << i);
					if (process->signal_handlers[i]) {
						exec_signal_handler(process->signal_stack,
							process->signal_handlers[i],
							&process->presignal_sp, i);
					}
					__asm__ ("msr DAIFSet, 0xf\nisb");
				}
			}
			page_free(process->signal_stack);
			__asm__ ("msr DAIFSet, 0xf\nisb");
			process->signal_stack = NULL;
		}
	}
}


void handle_sync(struct trapframe *trapframe) {
	uint64_t esr_el1;
	__asm__ ("mrs %0, esr_el1" : "=r" (esr_el1));

	if ((esr_el1 & ESR_EL1_EC_MASK) == ESR_EL1_EC_SVC_AARCH64) {
		__asm__ ("isb\nmsr DAIFClr, 0xf");
		syscall(trapframe);
		__asm__ ("msr DAIFSet, 0xf\nisb");
		struct kthread_states *states;
		__asm__ ("mrs %0, tpidr_el1" : "=r" (states));
		struct process_states *process = states->data;
		if (process->pending_signals && !process->signal_stack) {
			process->signal_stack = page_alloc(1);
			for (int i = 1; i <= SIGNAL_MAX; i++) {
				if ((process->pending_signals & (1 << i)) == (1 << i)) {
					process->pending_signals ^= (1 << i);
					if (process->signal_handlers[i]) {
						exec_signal_handler(process->signal_stack,
							process->signal_handlers[i],
							&process->presignal_sp, i);
					}
					__asm__ ("msr DAIFSet, 0xf\nisb");
				}
			}
			page_free(process->signal_stack);
			__asm__ ("msr DAIFSet, 0xf\nisb");
			process->signal_stack = NULL;
		}
	} else {
		handle_unimplemented();
	}
}
