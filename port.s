.section .text
.global outb
.global inb
.global inw
.global insl

.type outb, @function
    # void outb(uint16_t port, uint8_t value)
outb:

    movzwl	4(%esp), %edx
    movzbl	8(%esp), %eax

    outb %al, %dx
    ret

.size outb, . - outb

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

.type insl, @function
    # void insl(uint16_t port, void *addr, unsigned int count)
insl:
    pushl   %edi
    movl    8(%esp), %edx  # port
    movl    12(%esp), %edi # addr
    movl    16(%esp), %ecx # count

    xorl    %eax, %eax
.startLoop:
    cmpl    %eax, %ecx
    je      .end

    insl

    addl    $4, %edi
    incl    %eax
    jmp     .startLoop

.end:
    popl    %edi
    ret
