	.global sigkill_default
	.type sigkill_default, %function
sigkill_default:
	mov x8, 5
	svc 0

call_sigreturn:
	mov x8, 10
	svc 0

	.global exec_signal_handler
	.type exec_signal_handler, %function
exec_signal_handler:
	sub sp, sp, [16 * 7]
	stp x19, x20, [sp, 16 * 0]
	stp x21, x22, [sp, 16 * 1]
	stp x23, x24, [sp, 16 * 2]
	stp x25, x26, [sp, 16 * 3]
	stp x27, x28, [sp, 16 * 4]
	stp x29, x30, [sp, 16 * 5]
	mrs x5, sp_el0
	str x5, [sp, 16 * 6]
	mov x5, sp
	str x5, [x2]
	msr elr_el1, x1
	msr sp_el0, x0
	msr spsr_el1, xzr
	adr lr, call_sigreturn
	mov x0, x3
	dsb ish
	msr ttbr0_el1, x4
	tlbi vmalle1is
	dsb ish
	isb
	eret

	.global restore_after_signal
	.type restore_after_signal, %function
restore_after_signal:
	mov sp, x0
	ldp x19, x20, [sp, 16 * 0]
	ldp x21, x22, [sp, 16 * 1]
	ldp x23, x24, [sp, 16 * 2]
	ldp x25, x26, [sp, 16 * 3]
	ldp x27, x28, [sp, 16 * 4]
	ldp x29, x30, [sp, 16 * 5]
	ldr x2, [sp, 16 * 6]
	msr sp_el0, x2
	add sp, sp, [16 * 7]
	ret
