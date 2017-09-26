[bits 32]

section .text
global fastcp

fastcp:
    push    esi
    push    edi
    mov     edi, DWORD [esp + 12]
    mov     esi, DWORD [esp + 16]
    mov     ecx, DWORD [esp + 20]
    rep     movsb
    pop     edi
    pop     esi
    ret
