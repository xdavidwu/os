#include "aarch64/registers.h"
#include "exceptions.h"
#include "kthread.h"
#include "page.h"
#include "stdlib.h"

static int pid = 1;
static struct kthread_states *runq;
static struct kthread_states *zombies;

extern void kthread_switch(struct kthread_states *from, struct kthread_states *to);

static void kthread_wrap(void (*func)(void *), void *data) {
	func(data);
	kthread_exit();
}

void kthread_create(void (*func)(void *), void *data) {
	struct kthread_states *states = malloc(sizeof(struct kthread_states));
	DISABLE_INTERRUPTS();
	states->pid = pid++;
	ENABLE_INTERRUPTS();
	states->REGISTER_LR = (reg_t)kthread_wrap;
	states->REGISTER_P1 = (reg_t)func;
	states->REGISTER_P2 = (reg_t)data;
	states->stack_page = page_alloc(1);
	states->REGISTER_SP = (reg_t)states->stack_page + PAGE_UNIT;
	DISABLE_INTERRUPTS();
	if (runq) {
		struct kthread_states *ptrb = runq->prev;
		runq->prev->next = states;
		runq->prev = states;
		states->prev = ptrb;
		states->next = NULL;
	} else {
		runq = states;
		states->next = NULL;
		states->prev = runq;
	}
	states->next = runq;
	runq = states;
	ENABLE_INTERRUPTS();
}

void kthread_loop() {
	struct kthread_states *this = malloc(sizeof(struct kthread_states));
	this->pid = 0;
	__asm__ ("msr tpidr_el1, %0" : "=r" (this));
	while (1) {
		struct kthread_states *zombie = zombies;
		while (zombie) {
			page_free(zombie->stack_page);
			struct kthread_states *next = zombie->next;
			free(zombie);
			zombie = next;
		}
		zombies = NULL;
		kthread_yield();
	}
}

void kthread_yield() {
	struct kthread_states *states;
	__asm__ ("mrs %0, tpidr_el1" : "=r" (states));
	DISABLE_INTERRUPTS();
	if (runq) { // if not, should be reaper loop, and nothing pending
		// enq
		struct kthread_states *ptrb = runq->prev;
		runq->prev->next = states;
		runq->prev = states;
		states->prev = ptrb;
		states->next = NULL;
		// pop
		struct kthread_states *to = runq;
		runq = runq->next;
		runq->prev = runq;
		ENABLE_INTERRUPTS();
		kthread_switch(states, to);
	} else {
		ENABLE_INTERRUPTS();
	}
	return;
}

void kthread_exit() {
	struct kthread_states *states;
	__asm__ ("mrs %0, tpidr_el1" : "=r" (states));
	states->next = zombies;
	zombies = states;
}
