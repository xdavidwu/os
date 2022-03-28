#include <stdbool.h>
#include <stdint.h>
#include "errno.h"
#include "exceptions.h"

#define TIMERS_MAX	128

struct {
	uint64_t cval;
	void (*func)(void *);
	void *data;
} timers[TIMERS_MAX];

static uint64_t next_cval = UINT64_MAX;

int register_timer(int delay, void (*func)(void *), void *data) {
	bool ok = false;
	uint64_t freq, t, cval;
	asm("mrs %0, cntfrq_el0" : "=r" (freq));
	uint64_t enable = 1;
	asm("msr cntp_ctl_el0, %0": :"r" (enable));
	DISABLE_INTERRUPTS();
	for (int a = 0; a < TIMERS_MAX; a++) {
		if (!timers[a].cval) {
			ok = true;
			asm("mrs %0, cntpct_el0" : "=r" (t));
			cval = freq * delay + t;
			if (next_cval > cval) {
				asm("msr cntp_cval_el0, %0" : : "r" (cval));
				next_cval = cval;
			}
			timers[a].cval = cval;
			timers[a].func = func;
			timers[a].data = data;
			break;
		}
	}
	ENABLE_INTERRUPTS();
	if (!ok) {
		return -ENOSPC;
	}
	return SUCCESS;
}

void handle_timer() {
	uint64_t t;
	asm("mrs %0, cntpct_el0" : "=r" (t));
	next_cval = UINT64_MAX;
	for (int a = 0; a < TIMERS_MAX; a++) {
		if (timers[a].cval) {
			if (timers[a].cval <= t) {
				timers[a].cval = 0;
				timers[a].func(timers[a].data);
			} else if (next_cval > timers[a].cval) {
				next_cval = timers[a].cval;
			}
		}
	}
	asm("msr cntp_cval_el0, %0" : : "r" (next_cval));
}
