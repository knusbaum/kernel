[bits 32]

MALIGN   equ 1<<0
MEMINFO  equ 1<<1
FLAGS    equ MALIGN | MEMINFO
MAGIC    equ 0x1BADB002
CHECKSUM equ -(MAGIC + FLAGS)

section .multiboot
align 4
dd MAGIC
dd FLAGS
dd CHECKSUM


section .bootstrap_stack nobits write align=16
align 16
stack_bottom:
resb 32768
stack_top:

extern kernel_main
section .text
global _start:function (_start.end - _start)
_start:
    mov esp, stack_top
    ;sti
    push ebx
    call kernel_main
.end
global halt
halt:
    cli
    hlt
.Lhang:
    jmp halt
.end

section .text
global pause
pause:
    hlt
    ret

section .text
global sys_cli
sys_cli:
    hlt
    ret


section .text
global sys_sti
sys_sti:
    hlt
    ret

section .kend
global end_of_kernel
end_of_kernel:
