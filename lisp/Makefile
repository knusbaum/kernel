CFLAGS=-ggdb -m32 -O0 -Wall -Wextra -Werror -ffreestanding
EXECUTABLE=lisp
LDFLAGS=-lpthread -nostdlib -lgcc

OBJECTS=main.o \
		lexer.o \
		context.o \
		map.o \
		parser.o \
		object.o \
		lstring.o \
		threaded_vm.o \
		compiler.o \
		gc.o

all: $(OBJECTS)

lisp: $(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJECTS) -o $(EXECUTABLE)

clean:
	-@rm -Rf *~ *.o

nuke: clean
	-@rm -Rf $(EXECUTABLE)
