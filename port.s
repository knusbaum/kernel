.section .text
.global outb
.global inb
.global inw
.global call_putchar
	
.extern terminal_putchar
	
.type outb, @function
	# void outb(uint16_t port, uint8_t value)
outb:

	movzwl	4(%esp), %edx
	movzbl	8(%esp), %eax

	outb %al, %dx
	ret

.size outb, . - outb

call_putchar:
	pushl	'c'
	call	terminal_putchar
	addl	$4, %esp
	
	ret
	
.type inb, @function
	# uint8_t inb(uint16_t port)
inb:
	xorl	%eax, %eax
	movzwl	4(%esp), %edx
	inb	%dx, %al
	ret
	
.size inb, . - inb

.type inw, @function
	# uint16_t inw(uint16_t port)
inw:
	xorl	%eax, %eax
	movzwl	4(%esp), %edx
	inw	%dx, %ax
	ret
.size inw, . - inw
