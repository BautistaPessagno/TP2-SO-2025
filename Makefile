
all:  bootloader kernel userland image

bootloader:
	cd Bootloader; $(MAKE) all

kernel:
	cd Kernel; $(MAKE) MM_IMPL=$(MM_IMPL) all

userland:
	cd Userland; $(MAKE) all

image: kernel bootloader userland
	cd Image; $(MAKE) all

clean:
	cd Bootloader; $(MAKE) clean
	cd Image; $(MAKE) clean
	cd Kernel; $(MAKE) clean
	cd Userland; $(MAKE) clean
	rm -f *.zip

.PHONY: bootloader image collections kernel userland all clean
