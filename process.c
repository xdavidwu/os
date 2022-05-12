#include "aarch64/vmem.h"
#include "aarch64/registers.h"
#include "exceptions.h"
#include "kthread.h"
#include "page.h"
#include "process.h"
#include "stdlib.h"
#include <stdint.h>

extern void exec_user(void *stack, void *pc);

static void exec_wrap(struct process_states *states) {
	exec_user(states->page + PAGE_UNIT, states->image->page);
}

extern int kthread_dup(struct kthread_states *to, void *basek, void *newk,
	void *baseu, void *newu, int ret);

extern void sigkill_default();

int process_exec(uint8_t *image, size_t image_size) {
	struct process_states *process = malloc(sizeof(struct process_states));
	process->image = malloc(sizeof(struct process_image));
	int page_ord = 1;
	int pages = (image_size + PAGE_UNIT - 1) / PAGE_UNIT;
	while (pages > 1 << page_ord) {
		page_ord++;
	}
	uint8_t *ptr = page_alloc(page_ord);
	process->image->page = ptr;
	process->image->size = image_size;
	process->image->ref = 1;
	ptr += HIGH_MEM_OFFSET;
	while (image_size--) {
		*ptr++ = *image++;
	}
	process->page = page_alloc(1);
	for (int a = 0; a <= SIGNAL_MAX; a++) {
		process->signal_handlers[a] = NULL;
	}
	process->signal_handlers[SIGKILL] = sigkill_default;
	process->pending_signals = 0;
	process->presignal_sp = 0;
	process->signal_stack = NULL;
	return kthread_create((void (*)(void *))exec_wrap, process);
}

void process_exec_inplace(uint8_t *image, size_t image_size) {
	struct kthread_states *kthr;
	__asm__ ("mrs %0, tpidr_el1" : "=r" (kthr));
	struct process_states *process = kthr->data;
	if (!--process->image->ref) {
		page_free(process->image->page);
		free(process->image);
	}
	process->image = malloc(sizeof(struct process_image));
	int page_ord = 1;
	int pages = (image_size + PAGE_UNIT - 1) / PAGE_UNIT;
	while (pages > 1 << page_ord) {
		page_ord++;
	}
	uint8_t *ptr = page_alloc(page_ord);
	process->image->page = ptr;
	process->image->size = image_size;
	process->image->ref = 1;
	ptr += HIGH_MEM_OFFSET;
	while (image_size--) {
		*ptr++ = *image++;
	}
	exec_user(process->page + PAGE_UNIT, process->image->page);
}

extern int pid;
extern struct kthread_states *runq;

int process_dup() {
	struct kthread_states *kthr;
	__asm__ ("mrs %0, tpidr_el1" : "=r" (kthr));
	struct process_states *process = kthr->data;
	process->image->ref++;
	register uint8_t *ptr = page_alloc(1), *old = process->page, *ptrk = page_alloc(1) + HIGH_MEM_OFFSET, *oldk = kthr->stack_page;
	struct process_states *new = malloc(sizeof(struct process_states));
	new->page = ptr;
	new->image = process->image;
	struct kthread_states *states = malloc(sizeof(struct kthread_states));
	states->data = new;
	states->x0 = 0;
	states->stack_page = ptrk;
	// TODO handle fork from signal handlers?
	new->pending_signals = process->pending_signals;
	new->presignal_sp = 0;
	new->signal_stack = NULL;
	for (int a = 0; a <= SIGNAL_MAX; a++) {
		new->signal_handlers[a] = process->signal_handlers[a];
	}
	__asm__ ("msr DAIFSet, 0xf\nisb");
	int mpid = pid++;
	states->pid = mpid;
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
	register int sz = PAGE_UNIT;
	ptr += HIGH_MEM_OFFSET;
	old += HIGH_MEM_OFFSET;
	while (sz--) {
		*ptr++ = *old++;
	}
	sz = PAGE_UNIT;
	while (sz--) {
		*ptrk++ = *oldk++;
	}
	return kthread_dup(states, kthr->stack_page, states->stack_page, process->page, new->page, mpid);
}

void process_exit() {
	struct kthread_states *kthr;
	__asm__ ("mrs %0, tpidr_el1" : "=r" (kthr));
	struct process_states *process = kthr->data;
	if (!--process->image->ref) {
		page_free(process->image->page);
		free(process->image);
	}
	page_free(process->signal_stack);
	page_free(process->page);
	free(process);
	kthread_exit();
}
