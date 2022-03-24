	.global exec_user
	.type exec_user, %function
exec_user:
	msr elr_el1, x0
	msr sp_el0, x0
	mov x0, 0x3c0
	msr spsr_el1, x0
	eret
