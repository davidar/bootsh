ARCH = $(shell uname -m)
NPROC = $(shell nproc)
MAKEFLAGS += -j$(NPROC)

.PHONY: gawk binutils bzip2 linux-headers bash muxzcat perl bearssl curl test-host sabotage-stage0 sabotage-stage1 sabotage

/src/tarballs/gawk-5.3.0.tar.xz:
	@echo "Downloading gawk source code..."
	wget http://ftp.gnu.org/gnu/gawk/gawk-5.3.0.tar.xz -O $@

gawk-5.3.0: /src/tarballs/gawk-5.3.0.tar.xz
	tar -xf $<

/usr/local/bin/gawk: gawk-5.3.0
	cd $< && ./configure --disable-shared LD=cc && $(MAKE) && $(MAKE) install
	ln -sf /usr/local/bin/gawk /usr/local/bin/awk

gawk: /usr/local/bin/gawk

/src/tarballs/binutils-2.27.tar.bz2:
	@echo "Downloading binutils source code..."
	wget http://ftp.gnu.org/gnu/binutils/binutils-2.27.tar.bz2 -O $@

binutils-2.27: /src/tarballs/binutils-2.27.tar.bz2
	tar -xf $<

/usr/local/bin/ar: binutils-2.27
	cd $< && ./configure --disable-gprofng CFLAGS=-D__LITTLE_ENDIAN__=1 && \
		$(MAKE) MAKEINFO=true && $(MAKE) MAKEINFO=true install

binutils: /usr/local/bin/ar

/src/tarballs/bzip2-1.0.8.tar.gz:
	@echo "Downloading bzip2 source code..."
	wget http://mirrors.kernel.org/sourceware/bzip2/bzip2-1.0.8.tar.gz -O $@

bzip2-1.0.8: /src/tarballs/bzip2-1.0.8.tar.gz
	tar -xf $<

/usr/local/bin/bzip2: bzip2-1.0.8
	cd $< && $(MAKE) CC=cc && $(MAKE) install

bzip2: /usr/local/bin/bzip2

/src/tarballs/linux-headers-4.19.88.tar.xz:
	@echo "Downloading linux-headers source code..."
	wget http://ftp.barfooze.de/pub/sabotage/tarballs/linux-headers-4.19.88.tar.xz -O $@

linux-headers-4.19.88: /src/tarballs/linux-headers-4.19.88.tar.xz
	tar -xf $<

/usr/local/include/linux: linux-headers-4.19.88
	cd $< && $(MAKE) install ARCH=$(shell uname -m)

linux-headers: /usr/local/include/linux

/src/tarballs/bash-5.2.21.tar.gz:
	@echo "Downloading bash source code..."
	wget http://ftp.gnu.org/gnu/bash/bash-5.2.21.tar.gz -O $@

bash-5.2.21: /src/tarballs/bash-5.2.21.tar.gz
	tar -xf $<

/bin/bash: bash-5.2.21
	cd $< && ./configure --prefix=/ --without-bash-malloc LD=cc && $(MAKE) && $(MAKE) install

bash: /bin/bash

muxzcat: muxzcat.c
	cc -o /usr/local/bin/xzcat $<

/src/tarballs/perl-5.38.2.tar.xz:
	@echo "Downloading perl source code..."
	wget http://mirrors.kernel.org/CPAN/src/5.0/perl-5.38.2.tar.xz -O $@

perl-5.38.2: /src/tarballs/perl-5.38.2.tar.xz
	tar -xf $<

/usr/local/bin/perl: perl-5.38.2
	cd $< && ./Configure -des -Uusenm -Uusedl -DEBUGGING=both && \
		$(MAKE) && $(MAKE) install

perl: /usr/local/bin/perl

/src/tarballs/bearssl-0.6.tar.gz:
	@echo "Downloading bearssl source code..."
	wget http://tar.sabo.xyz/bearssl/bearssl-0.6.tar.gz -O $@

bearssl-0.6: /src/tarballs/bearssl-0.6.tar.gz
	tar -xf $<

/usr/local/lib/libbearssl.a: bearssl-0.6
	cd $< && $(MAKE) && mkdir -p /usr/local/lib /usr/local/include && \
		cp -f build/libbearssl.a /usr/local/lib/ && cp -f inc/*.h /usr/local/include/

bearssl: /usr/local/lib/libbearssl.a

/src/tarballs/curl-8.0.1.tar.xz:
	@echo "Downloading curl source code..."
	wget http://tar.sabo.xyz/curl/curl-8.0.1.tar.xz -O $@

curl-8.0.1: /src/tarballs/curl-8.0.1.tar.xz
	tar -xf $<

/usr/local/bin/curl: curl-8.0.1 /usr/local/lib/libbearssl.a
	cd $< && ./configure --disable-docs --disable-manual --disable-shared --with-bearssl LD=cc && \
		$(MAKE) && $(MAKE) install

curl: /usr/local/bin/curl

test-host: bash
	cd test/cc && $(MAKE) && $(MAKE) clean
	cd lib/toybox && TEST_HOST=1 USER=root scripts/test.sh && rm -rf generated/testdir
	@echo "All tests passed!"

sabotage-%.tar.gz: /usr/local/bin/curl
	@echo "Downloading sabotage source code..."
	curl -Lk https://github.com/sabotage-linux/sabotage/archive/$*.tar.gz > $@

sabotage-%: sabotage-%.tar.gz
	tar -xf $<

sabotage-stage0: sabotage-c7a8f2051c17501a6bfef1d8a0a3fc9ab6103761 /usr/local/bin/gawk /usr/local/bin/ar /usr/local/bin/bzip2 /usr/local/bin/curl /usr/local/include/linux
	mkdir -p /sabotage
	mv -f $< /sabotage/src
	cd /sabotage/src && \
		cp KEEP/config.stage0 config && \
		sed -e "s@SABOTAGE_BUILDDIR=.*@SABOTAGE_BUILDDIR=/sabotage@" \
			-e "s@CC=.*@CC=cc@" -e "s@HOSTCC=.*@HOSTCC=cc@" \
			-e "s@MAKE_THREADS=1@MAKE_THREADS=$(NPROC)@" \
			-i config && cat config && \
		utils/setup-rootfs.sh && \
		CONFIG=/sabotage/src/config BUTCHDB=/sabotage/var/lib/butch.db DEPS=build:stage0 KEEP/bin/butch install stage0 && \
		echo "export A=$(ARCH)" > config && cat KEEP/config.stage1 >> config && \
		sed -i "s@MAKE_THREADS=1@MAKE_THREADS=$(NPROC)@" config && cat config

sabotage-stage1: sabotage-stage0
	cd / && rm -rf root include lib local tmp usr && \
		rm $(shell find /bin -type f -not -name sh) && \
		mv /sabotage/etc/hostname /etc/hostname.bak && \
		mv /sabotage/etc/hosts /etc/hosts.bak && \
		mv /sabotage/etc/resolv.conf /etc/resolv.conf.bak && \
		mv /sabotage/bin/* /bin && mv /sabotage/etc/* /etc && \
		rmdir /sabotage/bin /sabotage/etc /sabotage/dev /sabotage/proc /sabotage/sys && \
		mv -f /sabotage/* / && \
		rmdir /sabotage
	# strip --strip-unneeded /opt/stage0-musl/lib/*.a /opt/stage0-musl/lib/*.o /opt/stage0-musl/lib/*.so
	# ranlib /opt/stage0-musl/lib/*.a
	butch install stage1

sabotage: sabotage-stage1
	/src/utils/clean-stage1.sh
	/src/utils/rebuild-stage1.sh
