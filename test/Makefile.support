%: %.tar.*
	tar -xf $<

gawk: gawk-5.3.0
	cd $< && ./configure --disable-shared LD=cc && $(MAKE) && $(MAKE) install

binutils: binutils-2.39
	cd $< && ./configure --disable-gprofng CFLAGS=-D__LITTLE_ENDIAN__=1 && \
		$(MAKE) MAKEINFO=true && $(MAKE) MAKEINFO=true install

bzip2: bzip2-1.0.8 binutils
	cd $< && $(MAKE) CC=cc AR=/usr/local/bin/ar && $(MAKE) install

linux-headers: linux-headers-4.19.88-2
	cd $< && $(MAKE) install ARCH=$(shell uname -m)

bash: bash-5.2.21
	cd $< && ./configure --prefix=/ --without-bash-malloc LD=cc && $(MAKE) && $(MAKE) install
