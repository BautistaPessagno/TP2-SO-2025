include ../Makefile.inc

MODULE=shell.bin
SOURCES=$(wildcard [^_]*.c)
TEST_SOURCES=$(wildcard ../Tests/*.c)

all: $(MODULE)

$(MODULE): $(SOURCES) $(TEST_SOURCES)
	$(GCC) $(GCCFLAGS) -T shellModule.ld _loader.c -L../ $(SOURCES) $(TEST_SOURCES) -l:libc.a -l:libsys.a -o ../$(MODULE)

clean:
	rm -rf *.o

.PHONY: all clean print
