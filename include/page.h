#ifndef PAGE_H
#define PAGE_H

void page_alloc_init();
void *page_alloc(int ord);
void page_free(void *page);

#endif
