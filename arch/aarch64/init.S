_start:
	mrs x0, mpidr_el1
	and x0, x0, 3
	cbz x0, cpu1
loop:
	wfe
	b loop
cpu1:
	ldr x0, =_start
	mov sp, x0
	ldr x0, =__bss_start
	ldr x1, =__bss_end
clear_bss:
	str xzr, [x0], #8
	cmp x0, x1
	blo clear_bss
	bl machine_init
