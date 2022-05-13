	.global exec_user
	.type exec_user, %function
exec_user:
	msr DAIFSet, 0xf
	isb
	msr elr_el1, x1
	msr sp_el0, x0
	msr spsr_el1, xzr
	dsb ish
	msr ttbr0_el1, x2
	tlbi vmalle1is
	dsb ish
	isb
	msr DAIFClr, 0xf
	eret
