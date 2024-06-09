.POSIX:
.PHONY: all clean install

all: src/samu/samu
	$<

src/samu/samu:
	$(MAKE) -C src/samu

clean:
	rm -rf build lib/toybox/generated

install: all
	./install.sh

rootfs.cpio.xz:
	cd rootfs && find . | cpio -H newc -o | xz --check=crc32 > ../rootfs.cpio.xz

run: rootfs.cpio.xz
	qemu-system-i386 -kernel bzImage -initrd rootfs.cpio.xz -m 1G -nographic -append console=ttyS0
