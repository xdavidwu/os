	.global exec_user
	.type exec_user, %function
exec_user:
	msr DAIFSet, 0xf
	isb
	mov x1, 0
	ldr x2, =0xfffffffff000
	msr elr_el1, x1
	msr sp_el0, x2
	msr spsr_el1, xzr
	dsb ish
	msr ttbr0_el1, x0
	tlbi vmalle1is
	dsb ish
	isb
	msr DAIFClr, 0xf
	eret
