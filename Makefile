CC=cc
AS=as
KERNEL_IMG=myos.bin

CFLAGS = -ggdb -m32 -O2 -Wall -Wextra -std=gnu99 -ffreestanding
AFLAGS = --32 -ggdb
LDFLAGS = $(CFLAGS) -nostdlib -lgcc -Wl,--build-id=none

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

nuke: clean
	-@rm $(KERNEL_IMG) *.d

run: $(KERNEL_IMG)
	qemu-system-i386 --kernel $(KERNEL_IMG) -drive file=f32_active.disk,format=raw -m size=4096

run-kvm: $(KERNEL_IMG)
	sudo qemu-system-i386 --kernel $(KERNEL_IMG) -drive file=f32_active.disk,format=raw -m size=4096 --enable-kvm

$(KERNEL_IMG) : $(OBJECTS) linker.ld
	$(CC) $(LDFLAGS) -T linker.ld -o myos.bin $(OBJECTS)

## Realmode uses nasm
realmode.o : realmode.s
	nasm -f elf32 realmode.s -o realmode.o

## Generic assembly rule
%.o: %.s
	$(AS) $(AFLAGS) $< -o $@

deps/%.d : %.c
	@mkdir -p deps
	@mkdir -p `dirname $@`
	@echo -e "[MM]\t\t" $@
	@$(CC) $(CFLAGS) -MM $< -MF $@
