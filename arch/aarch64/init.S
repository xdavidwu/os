_start:
	mrs x1, mpidr_el1
	and x1, x1, 3
	cbz x1, cpu1
loop:
	wfe
	b loop
cpu1:
	bl el1
	ldr x1, =_start
	mov sp, x1
	ldr x1, =__bss_start
	ldr x2, =__bss_end
clear_bss:
	str xzr, [x1], #8
	cmp x1, x2
	blo clear_bss
	ldr x1, =fdt
	str x0, [x1]
	bl machine_init
el1:
	mov x1, (1 << 31) // HCR_EL2.RW
	msr hcr_el2, x1
	mov x1, 0x3c5 // D A I F EL1h
	msr spsr_el2, x1
	msr elr_el2, lr
	eret
