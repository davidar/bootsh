#!/bin/sh

qemu-system-i386 -kernel build/bzImage -initrd build/initramfs.cpio.xz -append ip=dhcp -m 1G -nographic -nic user
