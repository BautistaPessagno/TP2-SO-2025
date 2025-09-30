include ../Makefile.inc

MODULE=libsys.a
SOURCES_C=$(wildcard *.c)
SOURCES_ASM=$(wildcard asm/*.asm)

OBJECTS_C=$(SOURCES_C:.c=.o)
OBJECTS_ASM=$(SOURCES_ASM:.asm=.o)

all: $(MODULE)

$(MODULE): $(OBJECTS_C) $(OBJECTS_ASM)
	$(AR) rcs ../$@ $^

%.o : %.c
	$(GCC) -c $(GCCFLAGS) $<

%.o : %.asm
	$(ASM) $(ASMFLAGS) $< -o $@

clean:
	rm -f *.o
	rm -f ../$(MODULE)

.PHONY: all clean print
