#ifndef TIMER_H
#define TIMER_H

void arm_timer();
void rearm_timer();
int register_timer(int delay, void (*func)(void *), void *data);
void handle_timer();

#endif
