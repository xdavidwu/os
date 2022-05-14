#ifndef PAGE_H
#define PAGE_H

#define PAGE_UNIT	(4 * 1024)

void page_alloc_preinit(void *end);
void mem_reserve(void *start, void *end);
void page_alloc_init();
void *page_alloc(int ord);
void page_take(void *page);
int page_check_ref(void *page);
void page_free(void *page);

void malloc_init();

#endif
