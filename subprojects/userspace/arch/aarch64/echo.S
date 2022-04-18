	.section ".text"
	.global _start
_start:
	sub sp, sp, 1
loop:
	mov x0, sp
	mov x1, 1
	mov x8, 1
	svc 0
	mov x0, sp
	mov x1, 1
	mov x8, 2
	svc 0
	b loop
