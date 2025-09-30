include Makefile.inc

all: libc libsys shell snake

libc:
	cd libc; make all

libsys:
	cd libsys; make all

shell:
	cd Shell; make

snake:
	cd Snake; make

clean:
	cd Shell; make clean
	cd Snake; make clean
	cd libc; make clean
	cd libsys; make clean
	rm -rf *.bin *.o *.so *.a

.PHONY: libc libsys shell snake all clean
