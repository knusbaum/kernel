CC=gcc
AS=as
KERNEL_IMG=myos.bin

CFLAGS = -ggdb -m32 -O0 -Wall -Wextra -std=gnu99 -ffreestanding
AFLAGS = -m32 -ggdb
LDFLAGS = $(CFLAGS) -nostdlib -lgcc -Wl,--build-id=none -lssp

## END CONFIGURABLE ##

## Gather the necessary assembly files
ASM_FILES=$(shell ls *.s)
ASM_OBJ=$(patsubst %.s,%.o,$(ASM_FILES))

## Gather the necessary C files
CFILES=$(shell ls *.c)
C_OBJ=$(patsubst %.c,%.o,$(CFILES))
DEPFILES=$(patsubst %.c,./deps/%.d,$(CFILES))

OBJECTS=$(ASM_OBJ) $(C_OBJ)

all: $(KERNEL_IMG)

## Depfiles contain inter-file dependencies
-include $(DEPFILES)

clean:
	-@rm *.o *~
	-@rm -R deps
	-@make -C lisp clean

nuke: clean
	-@rm $(KERNEL_IMG)
	-@rm f32.disk
	-@make -C lisp nuke

run: $(KERNEL_IMG) f32.disk
	qemu-system-i386 --kernel $(KERNEL_IMG) -drive file=f32.disk,format=raw -m size=4096


run-debug: $(KERNEL_IMG) f32.disk
	@echo "gdb target localhost:1234"
	qemu-system-i386 --kernel $(KERNEL_IMG) -drive file=f32.disk,format=raw -m size=4096 -S -s

boot.o : boot.nasm
	nasm -f elf32 boot.nasm -o boot.o

realmode.o : realmode.s
	nasm -f elf realmode.s -o realmode.o

common_asm.o : common_asm.nasm
	nasm -f elf common_asm.nasm -o common_asm.o

gdt_asm.o : gdt_asm.nasm
	nasm -f elf gdt_asm.nasm -o gdt_asm.o

idt_asm.o : idt_asm.nasm
	nasm -f elf idt_asm.nasm -o idt_asm.o

port.o : port.s
	$(AS) --32 -ggdb port.s -o port.o

run-kvm: $(KERNEL_IMG) f32.disk
	sudo qemu-system-i386 --kernel $(KERNEL_IMG) -drive file=f32.disk,format=raw -m size=4096 --enable-kvm

f32.disk:
	-rm f32.disk
	dd if=/dev/zero of=f32.disk bs=1M count=100
	mkfs.fat -F32 f32.disk -s 1

mount_disk: f32.disk
	mkdir -p fat32
	sudo mount -rw f32.disk fat32

populate_disk: mount_disk
	#sudo cp *.c *.h fat32
	#sudo cp -R deps fat32/
	#sudo mkdir -p fat32/foo/bar/baz/boo/dep/doo/poo/goo/
	#sudo cp common.h fat32/foo/bar/baz/boo/dep/doo/poo/goo/tood.txt
	sudo cp lisp/*.lisp fat32/
	sleep 1
	sudo umount fat32
	-@rm -Rf fat32

lisp_obj:
	make -C lisp all

$(KERNEL_IMG) : $(OBJECTS) lisp_obj linker.ld
	$(CC) $(LDFLAGS) -T linker.ld -o myos.bin $(OBJECTS) $(shell ls lisp/*.o)

## Realmode uses nasm
#realmode.o : realmode.s
#	nasm -f elf32 realmode.s -o realmode.o

## Generic assembly rule
#%.o: %.s
#	$(AS) $(AFLAGS) $< -o $@

deps/%.d : %.c
	@mkdir -p deps
	@mkdir -p `dirname $@`
	@echo -e "[MM]\t\t" $@
	@$(CC) $(CFLAGS) -MM $< -MF $@
