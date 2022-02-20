// GPPUD delay: 150 "cycles" (likely VC, which is 400Mhz on rpi3b+)
// rpi3b+ main processor: 1.4 Ghz A53
	.global	__gppud_delay
	.type __gppud_delay, %function
__gppud_delay:
	mov x9, #300
loop:
	subs x9, x9, #1
	cbnz x9, loop
	ret
