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
	pit.o
#	interrupt.o


all: $(KERNEL_IMG)

clean: 
	-@rm *.o *~

kernel.o : kernel.c terminal.h
	$(CC) -ggdb -m32 -c kernel.c -o kernel.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra

boot.o : boot.s
	$(AS) --32 -ggdb boot.s -o boot.o

interrupt.o : interrupt.s
	$(AS) --32 -ggdb interrupt.s -o interrupt.o

port.o : port.s
	$(AS) --32 -ggdb port.s -o port.o

terminal.o : terminal.c terminal.h
	$(CC) -m32 -c -ggdb terminal.c -o terminal.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra

$(KERNEL_IMG) : $(OBJECTS) linker.ld
	$(CC) -m32 -ggdb -T linker.ld -o myos.bin -ffreestanding -O2 -nostdlib $(OBJECTS) -lgcc



gdt_asm.o : gdt_asm.s
	$(AS) --32 -ggdb gdt_asm.s -o gdt_asm.o

gdt.o : gdt.c gdt.h
	$(CC) -m32 -ggdb -c gdt.c -o gdt.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra



idt_asm.o : idt_asm.s
	$(AS) --32 -ggdb idt_asm.s -o idt_asm.o

idt.o : idt.c idt.h
	$(CC) -m32 -ggdb -c idt.c -o idt.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra


common.o : common.c common.h
	$(CC) -m32 -ggdb -c common.c -o common.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra


isr.o : isr.c isr.h
	$(CC) -m32 -ggdb -c isr.c -o isr.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra

pic.o : pic.c pic.h
	$(CC) -m32 -ggdb -c pic.c -o pic.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra

pit.o : pit.c pit.h
	$(CC) -m32 -ggdb -c pit.c -o pit.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra
