#include "aarch64/registers.h"
#include "kthread.h"
#include "page.h"
#include "process.h"
#include "stdlib.h"
#include <stdint.h>

extern void exec_user(void *addr);

static void exec_wrap(struct process_states *states) {
	exec_user(states->page + PAGE_UNIT);
}

int process_exec(uint8_t *image, size_t image_size) {
	struct process_states *process = malloc(sizeof(struct process_states));
	int page_ord = 1;
	int pages = (image_size + PAGE_UNIT - 1) / PAGE_UNIT + 1;
	while (pages > 1 << page_ord) {
		page_ord++;
	}
	uint8_t *ptr = page_alloc(page_ord);
	process->page = ptr;
	process->image_size = image_size;
	ptr += PAGE_UNIT;
	while (image_size--) {
		*ptr++ = *image++;
	}
	return kthread_create((void (*)(void *))exec_wrap, process);
}

void process_exit() {
	struct kthread_states *kthr;
	__asm__ ("mrs %0, tpidr_el1" : "=r" (kthr));
	struct process_states *states = kthr->data;
	page_free(states->page);
	free(states);
	kthread_exit();
}
