# Declare constants used for creating a multiboot header.
.set ALIGN, 1<<0		# align loaded modules on page boundaries
.set MEMINFO, 1<<1		# provide memory map
.set FLAGS, ALIGN | MEMINFO     # this is the Multiboot 'flag' field
.set MAGIC, 0x1BADB002		# 'magic number' lets bootloader find the header
.set CHECKSUM, -(MAGIC + FLAGS) # checksum of above, to prove we are multiboot

# Declare a header as in the Multiboot Standard. We put this into a special
# section so we can force the header to be in the start of the final program.
# You don't need to understand all these details as it is just magic values that
# is documented in the multiboot standard. The bootloader will search for this
# magic sequence and recognize us as a multiboot kernel.
.section .multiboot
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

# Currently the stack pointer register (esp) points at anything and using it may
# cause massive harm. Instead, we'll provide our own stack. We will allocate
# room for a small temporary stack by creating a symbol at the bottom of it,
# then allocating 16384 bytes for it, and finally creating a symbol at the top.
.section .bootstrap_stack, "aw", @nobits
stack_bottom:
.skip 32768         #32 KiB
stack_top:

.section .text
.global _start
.type _start, @function
_start:
    # Welcome to kernel mode! We now have sufficient code for the bootloader to
    # load and run our operating system. It doesn't do anything interesting yet.
    # Perhaps we would like to call printf("Hello, World\n"). You should now
    # realize one of the profound truths about kernel mode: There is nothing
    # there unless you provide it yourself. There is no printf function. There
    # is no <stdio.h> header. If you want a function, you will have to code it
    # yourself. And that is one of the best things about kernel development:
    # you get to make the entire system yourself. You have absolute and complete
    # power over the machine, there are no security restrictions, no safe
    # guards, no debugging mechanisms, there is nothing but what you build.

    # By now, you are perhaps tired of assembly language. You realize some
    # things simply cannot be done in C, such as making the multiboot header in
    # the right section and setting up the stack. However, you would like to
    # write the operating system in a higher level language, such as C or C++.
    # To that end, the next task is preparing the processor for execution of
    # such code. C doesn't expect much at this point and we only need to set up
    # a stack. Note that the processor is not fully initialized yet and stuff
    # such as floating point instructions are not available yet.

    # To set up a stack, we simply set the esp register to point to the top of
    # our stack (as it grows downwards).
    movl $stack_top, %esp

    # We are now ready to actually execute C code. We cannot embed that in an
    # assembly file, so we'll create a kernel.c file in a moment. In that file,
    # we'll create a C entry point called kernel_main and call it here.
    sti
    pushl %ebx       # EBX contains a pointer to the multiboot info structure.
    call kernel_main

    # In case the function returns, we'll want to put the computer into an
    # infinite loop. To do that, we use the clear interrupt ('cli') instruction
    # to disable interrupts, the halt instruction ('hlt') to stop the CPU until
    # the next interrupt arrives, and jumping to the halt instruction if it ever
    # continues execution, just to be safe. We will create a local label rather
    # than real symbol and jump to there endlessly.
.global halt
halt:
    cli
    hlt
.Lhang:
    jmp .Lhang

.section .text
.global pause
.type pause @function
pause:
    hlt
    ret

.section .text
.global sys_cli
.type sys_cli @function
sys_cli:
    hlt
    ret


.section .text
.global sys_sti
.type sys_sti @function
sys_sti:
    hlt
    ret
    
# Set the size of the _start symbol to the current location '.' minus its start.
# This is useful when debugging or when you implement call tracing.
.size _start, . - _start

.section .kend
.global end_of_kernel
end_of_kernel:
