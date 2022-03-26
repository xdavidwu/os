	.global rearm_timer
	.type rearm_timer, %function
	.global arm_timer
	.type arm_timer, %function
arm_timer:
	mov x0, 1
	msr cntp_ctl_el0, x0
rearm_timer:
	mrs x0, cntfrq_el0
	lsl x0, x0, 1
	msr cntp_tval_el0, x0
	ret

