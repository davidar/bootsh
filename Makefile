SYSROOT = $(CURDIR)/rootfs/sysroot
SEED = $(CURDIR)/rootfs/seed

TCC_CONFIG = --ar=ar --prefix=/ --config-ldl=no --config-debug=yes --config-bcheck=no --config-backtrace=no

.PHONY: all seed test diff bootstrap

all: clean seed test diff

clean:
	$(MAKE) -C lib/tcc clean
	$(MAKE) -C lib/musl clean
	$(MAKE) -C lib/toybox clean
	$(MAKE) -C src clean

seed: clean
	rm -rf $(SYSROOT)
	cd lib/musl && ./configure --prefix=/ && \
		$(MAKE) CFLAGS="-Os -g" && $(MAKE) DESTDIR=$(SYSROOT) install
	cd lib/tcc && make clean && ./configure $(TCC_CONFIG) \
			--extra-cflags="-Wall -Os -nostdinc -nostdlib -I$(SYSROOT)/include -DCONFIG_TCC_STATIC" \
			--extra-ldflags="-nostdlib $(SYSROOT)/lib/crt1.o $(SYSROOT)/lib/libc.a -static" && \
		$(MAKE)
	echo "#define LIBTCC1A_LEN $$(wc -c < lib/tcc/libtcc1.a)" > src/libtcc1a.h
	gzip -9 < lib/tcc/libtcc1.a | od -Anone -vtx1 | \
		sed 's/ /,0x/g;1s/^,/static char libtcc1a_data[] = {\n /;$$s/.*/&};/' \
		>> src/libtcc1a.h
	cd lib/toybox && ln -sf ../../toybox.config .config && $(MAKE) \
			NOSTRIP=1 \
			CFLAGS="-Os -nostdinc -nostdlib -I$(SYSROOT)/include -I/usr/include -I/usr/include/x86_64-linux-gnu -g" \
			LDFLAGS="-nostdlib $(SYSROOT)/lib/crt1.o $(SYSROOT)/lib/libc.a -static"
	cd src && CFLAGS="-Os -nostdinc -I$(SYSROOT)/include" \
			LDFLAGS="-nostdlib -static" \
			LIBS="$(SYSROOT)/lib/crt1.o $(SYSROOT)/lib/libc.a" \
			$(MAKE)

	rm -rf $(SEED)
	mkdir -p $(SEED)/bin
	cp src/dash $(SEED)/bin/sh
	strip $(SEED)/bin/sh

test:
	$(MAKE) -C test

diff:
	diffoscope --exclude-directory-metadata=yes rootfs/bootstrap-1 rootfs/bootstrap-2

bootstrap:
	cd lib/tcc && $(MAKE) clean && ./configure --cc=cc $(TCC_CONFIG) && \
		$(MAKE) && $(MAKE) DESTDIR=/dest install
	# cd lib/musl && $(MAKE) clean && ./configure --prefix=/ CC=cc RANLIB=echo && \
	# 	$(MAKE) CFLAGS=-g && $(MAKE) DESTDIR=/dest install
	cd lib/toybox && $(MAKE) clean && $(MAKE)
	echo "#define LIBTCC1A_LEN $$(wc -c < /dest/lib/tcc/libtcc1.a)" > src/libtcc1a.h
	gzip -9 < /dest/lib/tcc/libtcc1.a | od -Anone -vtx1 | \
		sed 's/ /,0x/g;1s/^,/static char libtcc1a_data[] = {\n /;$$s/.*/&};/' \
		>> src/libtcc1a.h
	cd src && $(MAKE) clean && $(MAKE)
	mkdir -p /out/bin # /out/lib
	cp src/dash /out/bin/sh
	# cp -r /dest/include /out/include
	# cp /dest/lib/libc.a /dest/lib/crt?.o /out/lib/
