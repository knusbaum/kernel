CC=cc
AS=as
KERNEL_IMG=myos.bin
OBJECTS=terminal.o \
	kernel.o \
	boot.o \
	port.o
#	interrupt.o


all: $(KERNEL_IMG)

clean: 
	-@rm *.o *~

kernel.o : kernel.c terminal.h
	$(CC) -m32 -c kernel.c -o kernel.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra

boot.o : boot.s
	$(AS) --32 boot.s -o boot.o

interrupt.o : interrupt.s
	$(AS) --32 interrupt.s -o interrupt.o

port.o : port.s
	$(AS) --32 port.s -o port.o

terminal.o : terminal.c terminal.h
	$(CC) -m32 -c terminal.c -o terminal.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra

$(KERNEL_IMG) : $(OBJECTS) linker.ld
	$(CC) -m32 -T linker.ld -o myos.bin -ffreestanding -O2 -nostdlib $(OBJECTS) -lgcc
