.POSIX:
.PHONY: all clean install

all: src/samu/samu
	$<

src/samu/samu:
	$(MAKE) -C src/samu

clean:
	rm -rf build lib/toybox/generated

install: all
	scripts/install.sh

bzImage:
	cd linux-6.9.3 && cp -f ../linux-6.9.3.config .config && \
		make ARCH=x86 bzImage -j$(shell nproc) && cp -f arch/x86/boot/bzImage ..

rootfs.cpio.xz:
	cd rootfs && find . | cpio -H newc -o | xz --check=crc32 > ../rootfs.cpio.xz

run: bzImage rootfs.cpio.xz
	qemu-system-i386 -kernel bzImage -initrd rootfs.cpio.xz -append ip=dhcp -m 1G -nographic -nic user
