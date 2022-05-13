#include "aarch64/vmem.h"
#include "aarch64/registers.h"
#include "exceptions.h"
#include "kthread.h"
#include "page.h"
#include "process.h"
#include "vmem.h"
#include "stdlib.h"
#include <stdint.h>

extern void exec_user(void *pagetable);

static void exec_wrap(struct process_states *states) {
	exec_user(states->pagetable);
}

extern int kthread_dup(struct kthread_states *to, void *basek, void *newk, int ret);

extern void sigkill_default();
extern void pagetable_populate_device(uint64_t *pagetable);

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
	process->page = page_alloc(2);
	for (int a = 0; a <= SIGNAL_MAX; a++) {
		process->signal_handlers[a] = NULL;
	}
	process->signal_handlers[SIGKILL] = sigkill_default;
	process->pending_signals = 0;
	process->presignal_sp = 0;
	process->signal_stack = page_alloc(1);
	process->in_signal = false;
	process->pagetable = pagetable_new();
	pagetable_insert_range(process->pagetable, PAGETABLE_USER_X, process->image->page, 0, 1 << page_ord);
	pagetable_insert_range(process->pagetable, PAGETABLE_USER_W, process->page, (void *)0xffffffffb000, 4);
	pagetable_insert_range(process->pagetable, PAGETABLE_USER_W, process->signal_stack, process->signal_stack, 1);
	pagetable_populate_device(process->pagetable);
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
	pagetable_destroy(process->pagetable);
	process->pagetable = pagetable_new();
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
	pagetable_insert_range(process->pagetable, PAGETABLE_USER_X, process->image->page, 0, page_ord);
	pagetable_insert_range(process->pagetable, PAGETABLE_USER_W, process->page, (void *)0xffffffffb000, 4);
	pagetable_insert_range(process->pagetable, PAGETABLE_USER_W, process->signal_stack, process->signal_stack, 1);
	pagetable_populate_device(process->pagetable);
	exec_user(process->pagetable);
}

extern int pid;
extern struct kthread_states *runq;

int process_dup() {
	struct kthread_states *kthr;
	__asm__ ("mrs %0, tpidr_el1" : "=r" (kthr));
	struct process_states *process = kthr->data;
	process->image->ref++;
	register uint8_t *ptr = page_alloc(4), *old = process->page, *oldk = kthr->stack_page + HIGH_MEM_OFFSET;
	struct process_states *new = malloc(sizeof(struct process_states));
	new->page = ptr;
	new->image = process->image;
	struct kthread_states *states = malloc(sizeof(struct kthread_states));
	states->data = new;
	states->x0 = 0;
	states->stack_page = page_alloc(1);
	uint8_t *ptrk = states->stack_page + HIGH_MEM_OFFSET;
	// TODO handle fork from signal handlers?
	new->pending_signals = process->pending_signals;
	new->presignal_sp = 0;
	new->signal_stack = page_alloc(1);
	new->in_signal = false;
	for (int a = 0; a <= SIGNAL_MAX; a++) {
		new->signal_handlers[a] = process->signal_handlers[a];
	}
	new->pagetable = pagetable_new();
	pagetable_insert_range(new->pagetable, PAGETABLE_USER_X, new->image->page, 0, (new->image->size + PAGE_UNIT - 1) / PAGE_UNIT);
	pagetable_insert_range(new->pagetable, PAGETABLE_USER_W, new->page, (void *)0xffffffffb000, 4);
	pagetable_insert_range(new->pagetable, PAGETABLE_USER_W, new->signal_stack, new->signal_stack, 1);
	pagetable_populate_device(new->pagetable);
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
	register int sz = PAGE_UNIT * 4;
	ptr += HIGH_MEM_OFFSET;
	old += HIGH_MEM_OFFSET;
	while (sz--) {
		*ptr++ = *old++;
	}
	uint8_t *cptrk = ptrk, *coldk = oldk;
	sz = PAGE_UNIT;
	while (sz--) {
		*cptrk++ = *coldk++;
	}
	struct trapframe *new_trap = (struct trapframe *)((uint8_t *)process->trapframe - oldk + ptrk);
	new_trap->ttbr0_el1 = (uint64_t)new->pagetable;
	return kthread_dup(states, kthr->stack_page, states->stack_page, mpid);
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
	pagetable_destroy(process->pagetable);
	free(process);
	kthread_exit();
}
