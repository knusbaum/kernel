[bits 32]

%define ALIGN 1<<0
%define MEMINFO 1<<1
%define FLAGS ALIGN | MEMINFO
%define MAGIC 0x1BADB002
%define CHECKSUM -(MAGIC + FLAGS)

section .multiboot
align 4
dd MAGIC
dd FLAGS
dd CHECKSUM


section .bootstrap_stack
stack_bottom:
resb 32768
stack_top:

extern kernel_main

section .text
global _start
_start:
    mov esp, stack_top
    sti
    push ebx
    call kernel_main

global halt
halt:
    cli
    hlt
.Lhang:
    jmp halt


section .kend
global end_of_kernel
end_of_kernel: