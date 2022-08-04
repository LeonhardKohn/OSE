	.section .text
	.global _entry
_entry:
	//stack pointer initialization here	
	la	sp, userstack
	li	a0, 4096
	add	sp, sp, a0
	jal	main
goaway:
	li	a7, 42
	li	a0, 0
	ecall

