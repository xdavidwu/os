	.global kthread_switch
	.type kthread_switch, %function
kthread_switch:
	stp x19, x20, [x0, 16 * 1]
	stp x21, x22, [x0, 16 * 2]
	stp x23, x24, [x0, 16 * 3]
	stp x25, x26, [x0, 16 * 4]
	stp x27, x28, [x0, 16 * 5]
	stp x29, x30, [x0, 16 * 6]
	mov x3, sp
	str x3, [x0, 16 * 7]

	ldp x19, x20, [x1, 16 * 1]
	ldp x21, x22, [x1, 16 * 2]
	ldp x23, x24, [x1, 16 * 3]
	ldp x25, x26, [x1, 16 * 4]
	ldp x27, x28, [x1, 16 * 5]
	ldp x29, x30, [x1, 16 * 6]
	ldr x0, [x1, 16 * 7]
	mov sp, x0
	msr tpidr_el1, x1
	ldp x0, x1, [x1, 16 * 0]
	ret
