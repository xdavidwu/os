	.section ".text"
	.global _start
_start:
	mov x19, 0
1:
	add x19, x19, 1
	adr x0, str
	mov x1, 13
	mov x8, 2
	svc 0
	cmp x19, 5
	blt 1b
1:
	b 1b

	.section ".rodata"
str:
	.ascii "Hello World!\n"
