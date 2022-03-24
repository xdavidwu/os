#include "kio.h"

static void kput64x(uint64_t val) {
	char hexbuf[9];
	hexbuf[8] = '\0';
	for (int i = 0; i < 16; i++) {
		int dig = val & 0xf;
		hexbuf[7 - i] = (dig > 9 ? 'a' - 10 : '0') + dig;
		val >>= 4;
	}
	kputs(hexbuf);
}

void exception_handler(uint64_t spsr_el1, uint64_t elr_el1, uint64_t esr_el1) {
	kputs("spsr_el1:\t");
	kput64x(spsr_el1);
	kputs("\nelr_el1:\t");
	kput64x(elr_el1);
	kputs("\nesr_el1:\t");
	kput64x(esr_el1);
	kputc('\n');
}