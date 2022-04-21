#include "process.h"
#include "kthread.h"
#include "page.h"
#include "stdlib.h"
#include <stdint.h>

extern void exec_user(void *addr);

void process_exec(uint8_t *image, size_t image_size) {
	struct process_states *process = malloc(sizeof(struct process_states));
	int page_ord = 1;
	int pages = (image_size + PAGE_UNIT - 1) / PAGE_UNIT + 1;
	while (pages > 1 << page_ord) {
		page_ord++;
	}
	uint8_t *ptr = page_alloc(page_ord) + PAGE_UNIT;
	process->image_page = ptr;
	process->image_size = image_size;
	while (image_size--) {
		*ptr++ = *image++;
	}
	kthread_create(exec_user, process->image_page);
}
