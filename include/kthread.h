#ifndef KTHREAD_H
#define KTHREAD_H

int kthread_create(void (*func)(void *), void *data);

void kthread_yield();

void kthread_exit();

void kthread_wait(int id);

#endif
