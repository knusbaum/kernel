.section .text
.global outb
.global inb
.global inw
.global insw
.global outsw

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

.type insw, @function
    # void insw(uint16_t port, void *addr, unsigned int count)
insw:
    pushl   %edi
    movl    8(%esp), %edx  # port
    movl    12(%esp), %edi # addr
    movl    16(%esp), %ecx # count

    xorl    %eax, %eax
.insw_startLoop:
    cmpl    %eax, %ecx
    je      .insw_end

    insw

    incl    %eax
    jmp     .insw_startLoop

.insw_end:
    popl    %edi
    ret

.type outsw, @function
    # void outsw(uint16_t port, void *addr, unsigned int count)
outsw:
    pushl   %esi
    movl    8(%esp), %edx  # port
    movl    12(%esp), %esi # addr
    movl    16(%esp), %ecx # count

    xorl    %eax, %eax
.outsw_startLoop:
    cmpl    %eax, %ecx
    je      .outsw_end

    outsw

    incl    %eax
    jmp     .outsw_startLoop

.outsw_end:
    popl    %esi
    ret
