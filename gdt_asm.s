.section .text
.global load_gdt

.type load_gdt, @function
    # void load_gdt(gdt_ptr_t * gdt_ptr)
load_gdt:
    movl    4(%esp), %eax
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


.size load_gdt, . - load_gdt
