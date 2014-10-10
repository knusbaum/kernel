CC=cc
AS=as

CFLAGS= -ggdb -m32 -std=gnu99 -ffreestanding -O2 -Wall -Wextra
ASFLAGS= --32 -ggdb
LDFLAGS= -m32 -ggdb -ffreestanding -O2 -nostdlib -lgcc

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
	kheap.o \
	frame.o \
	paging.o


all: $(KERNEL_IMG)

run: $(KERNEL_IMG)
	sudo cp $(KERNEL_IMG) /boot/
	sudo init 6

clean: 
	-@rm *.o *.d *~

$(KERNEL_IMG) : $(OBJECTS) linker.ld
	@$(CC) $(LDFLAGS) -T linker.ld -o $(KERNEL_IMG) $(OBJECTS) 
	-@echo -e [LD] '\t' $(KERNEL_IMG)

%.o : %.c %.h
	@$(CC) $(CFLAGS) -o $@ -c $<
	@$(CC) -MM $(CFLAGS) -c $< > $*.d
	-@echo -e [CC] '\t' $@

%.o : %.c
	@$(CC) $(CFLAGS) -o $@ -c $<
	@$(CC) -MM $(CFLAGS) -c $< > $*.d
	-@echo -e [CC] '\t' $@

%.o : %.s
	@$(AS) $(ASFLAGS) $< -o $@
	-@echo -e [AS] '\t' $@

-include $(OBJECTS:.o=.d)
