_start:
	mrs x1, mpidr_el1
	and x1, x1, 3
	cbz x1, cpu1
loop:
	wfe
	b loop
cpu1:
	bl el1
	ldr x1, =0x80100010
	msr tcr_el1, x1
	mov x1, 0b0000000001000100
	msr mair_el1, x1
	adr x1, pgd
	msr ttbr0_el1, x1
	msr ttbr1_el1, x1
	mov x1, 1
	msr sctlr_el1, x1
	ldr x2, =rest
	br x2
rest:
	msr ttbr0_el1, xzr
	ldr x1, =_start
	mov sp, x1
	ldr x1, =__bss_start
	ldr x2, =__bss_end
clear_bss:
	str xzr, [x1], #8
	cmp x1, x2
	blo clear_bss
	ldr x1, =0xffff000000000000
	add x0, x0, x1
	adr x1, fdt
	str x0, [x1]
	ldr x1, =__prepage_heap
	ldr x2, =prepage_heap
	str x1, [x2]
	mrs x0, cntkctl_el1
	orr x0, x0, 1
	msr cntkctl_el1, x0
	bl machine_init
el1:
	mov x1, (1 << 31) // HCR_EL2.RW
	msr hcr_el2, x1
	mov x1, 0x3c5 // D A I F EL1h
	msr spsr_el2, x1
	msr elr_el2, lr
	eret
