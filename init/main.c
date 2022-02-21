#include "init.h"
#include "kio.h"

void main() {
	kputs("Hello World!\n");
	while (1) {
		char buf[1024];
		kgets(buf, 1023);
		kputs(buf);
	}
}
