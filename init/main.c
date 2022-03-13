#include "fdt.h"
#include "init.h"
#include "kshell.h"

void main() {
	fdt_init();
	kshell();
}
