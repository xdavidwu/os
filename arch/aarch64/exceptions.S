.macro push_state
	sub sp, sp, 16 * 11
	stp x0, x1, [sp, 16 * 0]
	stp x2, x3, [sp, 16 * 1]
	stp x4, x5, [sp, 16 * 2]
	stp x6, x7, [sp, 16 * 3]
	stp x8, x9, [sp, 16 * 4]
	stp x10, x11, [sp, 16 * 5]
	stp x12, x13, [sp, 16 * 6]
	stp x14, x15, [sp, 16 * 7]
	stp x16, x17, [sp, 16 * 8]
	stp x18, x29, [sp, 16 * 9]
	str x30, [sp, 16 * 10]
.endm

.macro pop_state
	ldp x0, x1, [sp, 16 * 0]
	ldp x2, x3, [sp, 16 * 1]
	ldp x4, x5, [sp, 16 * 2]
	ldp x6, x7, [sp, 16 * 3]
	ldp x8, x9, [sp, 16 * 4]
	ldp x10, x11, [sp, 16 * 5]
	ldp x12, x13, [sp, 16 * 6]
	ldp x14, x15, [sp, 16 * 7]
	ldp x16, x17, [sp, 16 * 8]
	ldp x18, x29, [sp, 16 * 9]
	ldr x30, [sp, 16 * 10]
	add sp, sp, 16 * 11
.endm


_unimplemented_handler:
_sync_handler:
	push_state
	mrs x0, spsr_el1
	mrs x1, elr_el1
	mrs x2, esr_el1
	bl handle_unimplemented
	pop_state
	eret

_irq_handler:
	msr DAIFSet, 0xf
	push_state
	bl handle_irq
	pop_state
	msr DAIFClr, 0xf
	eret

	.align 11 // 0x800
vtable:
	# from EL1t
	b _unimplemented_handler
	.align 7 // 0x80
	b _unimplemented_handler
	.align 7 // 0x80
	b _unimplemented_handler
	.align 7 // 0x80
	b _unimplemented_handler
	.align 7 // 0x80

	# from EL1h
	b _unimplemented_handler
	.align 7 // 0x80
	b _irq_handler
	.align 7 // 0x80
	b _unimplemented_handler
	.align 7 // 0x80
	b _unimplemented_handler
	.align 7 // 0x80

	# from aarch64 EL0
	b _sync_handler
	.align 7 // 0x80
	b _irq_handler
	.align 7 // 0x80
	b _unimplemented_handler
	.align 7 // 0x80
	b _unimplemented_handler
	.align 7 // 0x80

	# from aarch32 EL0
	b _unimplemented_handler
	.align 7 // 0x80
	b _unimplemented_handler
	.align 7 // 0x80
	b _unimplemented_handler
	.align 7 // 0x80
	b _unimplemented_handler
	.align 7 // 0x80

	.global arm_exceptions
	.type arm_exceptions, %function

arm_exceptions:
	adr x0, vtable
	msr vbar_el1, x0
	msr DAIFClr, 0xf
	ret
