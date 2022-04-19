#include "fdt.h"
#include "init.h"
#include "kshell.h"
#include "kthread.h"
#include "string.h"

extern void kthread_loop();

void main() {
	kthread_create(kshell, NULL);
	kthread_loop();
}
