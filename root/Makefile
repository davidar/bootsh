MAKEFLAGS += -j$(shell nproc)

gawk-5.3.0: gawk-5.3.0.tar.xz muxzcat
	tar -xf $<

gawk: gawk-5.3.0
	cd $< && ./configure --disable-shared LD=cc && $(MAKE) && $(MAKE) install

binutils-2.27: binutils-2.27.tar.bz2 bzip2
	tar -xf $<

binutils: binutils-2.27
	cd $< && ./configure --disable-gprofng CFLAGS=-D__LITTLE_ENDIAN__=1 && \
		$(MAKE) MAKEINFO=true && $(MAKE) MAKEINFO=true install

bzip2-1.0.8.tar.gz:
	@echo "Downloading bzip2 source code..."
	wget http://mirrors.kernel.org/sourceware/bzip2/bzip2-1.0.8.tar.gz

bzip2-1.0.8: bzip2-1.0.8.tar.gz
	tar -xf $<

bzip2: bzip2-1.0.8
	cd $< && $(MAKE) CC=cc && $(MAKE) install

linux-headers-4.19.88: linux-headers-4.19.88.tar.xz muxzcat
	tar -xf $<

linux-headers: linux-headers-4.19.88
	cd $< && $(MAKE) install ARCH=$(shell uname -m)

bash-5.2.21.tar.gz:
	@echo "Downloading bash source code..."
	wget http://ftp.gnu.org/gnu/bash/bash-5.2.21.tar.gz

bash-5.2.21: bash-5.2.21.tar.gz
	tar -xf $<

bash: bash-5.2.21
	cd $< && ./configure --prefix=/ --without-bash-malloc LD=cc && $(MAKE) && $(MAKE) install

muxzcat: muxzcat.c
	cc -o /usr/local/bin/xzcat $<

sh: bootsh bash bzip2
	cd $< && HOSTCC=cc ./configure && $(MAKE) clean && $(MAKE) && mv -f bootsh /bin/sh
