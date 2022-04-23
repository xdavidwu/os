	.global kthread_switch
	.type kthread_switch, %function
	.global kthread_exit_next
	.type kthread_exit_next, %function
kthread_switch:
	stp x19, x20, [x0, 16 * 1]
	stp x21, x22, [x0, 16 * 2]
	stp x23, x24, [x0, 16 * 3]
	stp x25, x26, [x0, 16 * 4]
	stp x27, x28, [x0, 16 * 5]
	stp x29, x30, [x0, 16 * 6]
	mov x2, sp
	mrs x3, sp_el0
	stp x2, x3, [x0, 16 * 7]
kthread_exit_next:
	ldp x19, x20, [x1, 16 * 1]
	ldp x21, x22, [x1, 16 * 2]
	ldp x23, x24, [x1, 16 * 3]
	ldp x25, x26, [x1, 16 * 4]
	ldp x27, x28, [x1, 16 * 5]
	ldp x29, x30, [x1, 16 * 6]
	ldp x0, x2, [x1, 16 * 7]
	mov sp, x0
	msr sp_el0, x2
	msr tpidr_el1, x1
	ldp x0, x1, [x1, 16 * 0]
	msr DAIFClr, 0xf
	ret

	.global kthread_dup
	.type kthread_dup, %function
kthread_dup:
	stp x19, x20, [x0, 16 * 1]
	stp x21, x22, [x0, 16 * 2]
	stp x23, x24, [x0, 16 * 3]
	stp x25, x26, [x0, 16 * 4]
	stp x27, x28, [x0, 16 * 5]
	stp x29, x30, [x0, 16 * 6]
	mov x6, sp
	sub x6, x6, x1
	add x6, x6, x2
	mrs x7, sp_el0
	sub x7, x7, x3
	add x7, x7, x4
	stp x6, x7, [x0, 16 * 7]
	mov x0, x5
	msr DAIFClr, 0xf
	ret
