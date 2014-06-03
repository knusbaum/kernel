.section .text
.global gdt_flush

.type gdt_flush, @function
	# void gdt_flush(gdt_ptr_t * gdt_ptr)
gdt_flush:
	movl 	4(%esp), %eax
	lgdt	(%eax)

	# 0x10 is the offset in the GDT to our data segment
	movw	$0x10, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	%ax, %gs
	movw	%ax, %ss
	ljmp	$0x08, $.flush  # Long jump to our new code segment
.flush:
	ret

	
.size gdt_flush, . - gdt_flush
