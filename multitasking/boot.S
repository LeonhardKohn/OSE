	.section .text
	.global _entry
_entry:
    li      a0, 4096
	la      sp, kernelstack
	add     sp, sp, a0
	jal	setup
loop:
	j	loop

