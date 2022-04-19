#ifndef KTHREAD_H
#define KTHREAD_H

void kthread_create(void (*func)(void *), void *data);

void kthread_yield();

void kthread_exit();

#endif
