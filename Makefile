CC=cc
AS=as
KERNEL_IMG=myos.bin
OBJECTS=terminal.o \
	kernel.o \
	common.o \
	boot.o \
	port.o \
	gdt.o \
	gdt_asm.o \
	idt.o \
	idt_asm.o \
	isr.o \
	pic.o \
	pit.o \
	kmalloc_early.o \
	frame.o \
	paging.o
#	interrupt.o
CFLAGS = -ggdb -m32 -O0 -Wall -Wextra -std=gnu99 -ffreestanding
AFLAGS = --32 -ggdb
LDFLAGS = $(CFLAGS) -nostdlib -lgcc -Wl,--build-id=none

all: $(KERNEL_IMG)

#run: $(KERNEL_IMG)
#	sudo cp $(KERNEL_IMG) /boot/
#	sudo init 6

clean: 
	-@rm *.o *~

nuke: clean
	-@rm $(KERNEL_IMG) *.d

kernel.o : kernel.c terminal.h gdt.h idt.h pit.h pic.h
	$(CC) $(CFLAGS) -c kernel.c -o kernel.o

boot.o : boot.s
	$(AS) $(AFLAGS) boot.s -o boot.o

interrupt.o : interrupt.s
	$(AS) $(AFLAGS) interrupt.s -o interrupt.o

port.o : port.s
	$(AS) $(AFLAGS) port.s -o port.o

terminal.o : terminal.c terminal.h
	$(CC) $(CFLAGS) -c terminal.c -o terminal.o

$(KERNEL_IMG) : $(OBJECTS) linker.ld
	$(CC) $(LDFLAGS) -T linker.ld -o myos.bin $(OBJECTS)



gdt_asm.o : gdt_asm.s
	$(AS) $(AFLAGS) gdt_asm.s -o gdt_asm.o

gdt.o : gdt.c gdt.h terminal.h
	$(CC) $(CFLAGS) -c gdt.c -o gdt.o



idt_asm.o : idt_asm.s
	$(AS) $(AFLAGS) idt_asm.s -o idt_asm.o

idt.o : idt.c idt.h common.h terminal.h
	$(CC) $(CFLAGS) -c idt.c -o idt.o


common.o : common.c common.h
	$(CC) $(CFLAGS) -c common.c -o common.o


isr.o : isr.c isr.h terminal.h pic.h port.h
	$(CC) $(CFLAGS) -c isr.c -o isr.o

pic.o : pic.c pic.h port.h terminal.h
	$(CC) $(CFLAGS) -c pic.c -o pic.o

pit.o : pit.c pit.h isr.h port.h terminal.h
	$(CC) $(CFLAGS) -c pit.c -o pit.o

kmalloc_early.o : kmalloc_early.c kmalloc_early.h
	$(CC) $(CFLAGS) -c kmalloc_early.c -o kmalloc_early.o

frame.o : frame.c frame.h
	$(CC) $(CFLAGS) -c frame.c -o frame.o

paging.o : paging.c paging.h
	$(CC) $(CFLAGS) -c paging.c -o paging.o
