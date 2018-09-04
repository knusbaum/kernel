[bits 32]

section .text
global outb
global inb
global inw
global insw
global outsw

; void outb(uint16_t port, uint8_t value)
outb:
    movzx	edx, WORD [esp+4]
    movzx	eax, BYTE  [esp+8]

    out     dx, al
    ret

; uint8_t inb(uint16_t port)
inb:
    xor		eax, eax
    movzx	edx, WORD [esp+4]
    in      al, dx
    ret

; uint16_t inw(uint16_t port)
inw:
    xor		eax, eax
    movzx	edx, WORD [esp+4]
    in      ax, dx
    ret

; void insw(uint16_t port, void *addr, unsigned int count)
insw:
    push    edi
    mov     edx, [esp+8]        ; port
    mov     edi, [esp+12]       ; addr
    mov     ecx, [esp+16]       ; count

    xor     eax, eax
.insw_startLoop:
    cmp     eax, ecx
    je      .insw_end

    insw

    inc     eax
    jmp     .insw_startLoop

.insw_end:
    pop     edi
    ret

; void outsw(uint16_t port, void *addr, unsigned int count)
outsw:
    push    esi
    mov     edx, [esp+8]        ; port
    mov     esi, [esp+12]       ; addr
    mov     ecx, [esp+12]       ; count

    xor     eax, eax
.outsw_startLoop:
    cmp     eax, ecx
    je      .outsw_end

    outsw

    inc     eax
    jmp     .outsw_startLoop

.outsw_end:
    pop    esi
    ret
