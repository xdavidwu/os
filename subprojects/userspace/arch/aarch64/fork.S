	.section ".text"
	.global _start
_start:
	mov x8, 4
	svc 0
	mov x19, 0
	add x19, x19, 1
	adr x0, str
	mov x1, 13
	mov x8, 2
	svc 0
	mov x8, 5
	svc 0

	.section ".rodata"
str:
	.ascii "Hello World!\n"
