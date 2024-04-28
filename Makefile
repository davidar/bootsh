SEED0 = $(CURDIR)/rootfs/seed0
SEED = $(CURDIR)/rootfs/seed

TCC_CONFIG = --prefix=/ --config-ldl=no --config-debug=yes --config-bcheck=no --config-backtrace=no

.PHONY: all seed test diff bootstrap

all: clean seed test diff

clean:
	$(MAKE) -C lib/tcc clean
	$(MAKE) -C lib/musl clean
	$(MAKE) -C lib/toybox clean
	$(MAKE) -C src clean

seed:
	rm -rf $(SEED0)
	cd lib/tcc && ./configure $(TCC_CONFIG) && $(MAKE) && $(MAKE) DESTDIR=$(SEED0) install
	cd lib/musl && ./configure --prefix=/ --target=x86_64 \
			CC=$(SEED0)/bin/tcc \
			AR="$(SEED0)/bin/tcc -ar" \
			RANLIB=echo \
			LIBCC=$(SEED0)/lib/tcc/libtcc1.a && \
		$(MAKE) CFLAGS=-g && $(MAKE) DESTDIR=$(SEED0) install
	echo "GROUP ( $(SEED0)/lib/libc.a $(SEED0)/lib/tcc/libtcc1.a )" > $(SEED0)/libc.ld
	cd lib/tcc && make clean && ./configure $(TCC_CONFIG) \
			--extra-cflags="-nostdinc -nostdlib -I$(SEED0)/include -DCONFIG_TCC_STATIC" \
			--extra-ldflags="-nostdlib $(SEED0)/lib/crt1.o $(SEED0)/libc.ld -static" && \
		$(MAKE) && $(MAKE) DESTDIR=$(SEED0) install
	echo "#define LIBTCC1A_LEN $$(wc -c < $(SEED0)/lib/tcc/libtcc1.a)" > src/libtcc1a.h
	gzip -9 < $(SEED0)/lib/tcc/libtcc1.a | od -Anone -vtx1 | \
		sed 's/ /,0x/g;1s/^,/static char libtcc1a_data[] = {\n /;$$s/.*/&};/' \
		>> src/libtcc1a.h
	cd lib/toybox && ln -sf ../../toybox.config .config && $(MAKE) \
			NOSTRIP=1 \
			CC=$(SEED0)/bin/tcc \
			CFLAGS="-nostdinc -nostdlib -I$(SEED0)/include -I/usr/include -I/usr/include/x86_64-linux-gnu -g" \
			LDFLAGS="-nostdlib $(SEED0)/lib/crt1.o $(SEED0)/libc.ld -static"
	cd src && CC=$(SEED0)/bin/tcc \
			CFLAGS="-nostdinc -I$(SEED0)/include" \
			LDFLAGS="-nostdlib -static" \
			LIBS="$(SEED0)/lib/crt1.o $(SEED0)/libc.ld" \
			$(MAKE)

	rm -rf $(SEED)
	mkdir -p $(SEED)/bin
	cp src/dash $(SEED)/bin/sh
	cp -r $(SEED0)/include $(SEED)/include
	cp -r $(SEED0)/lib $(SEED)/lib
	rm -rf $(SEED)/lib/tcc

test:
	$(MAKE) -C test

diff:
	diffoscope --exclude-directory-metadata=yes rootfs/bootstrap-1 rootfs/bootstrap-2

bootstrap:
	cd lib/tcc && $(MAKE) clean && ./configure --cc=cc $(TCC_CONFIG) && \
		$(MAKE) && $(MAKE) DESTDIR=/dest install
	cd lib/musl && $(MAKE) clean && ./configure --prefix=/ CC=cc AR="cc -ar" RANLIB=echo && \
		$(MAKE) CFLAGS=-g && $(MAKE) DESTDIR=/dest install
	cd lib/toybox && $(MAKE) clean && $(MAKE)
	echo "#define LIBTCC1A_LEN $$(wc -c < /dest/lib/tcc/libtcc1.a)" > src/libtcc1a.h
	gzip -9 < /dest/lib/tcc/libtcc1.a | od -Anone -vtx1 | \
		sed 's/ /,0x/g;1s/^,/static char libtcc1a_data[] = {\n /;$$s/.*/&};/' \
		>> src/libtcc1a.h
	cd src && $(MAKE) clean && $(MAKE)
	mkdir -p /out/bin
	cp src/dash /out/bin/sh
	cp -r /dest/include /out/include
	cp -r /dest/lib /out/lib
	rm /out/lib/*.so*
	rm /out/lib/libtcc.a
	rm -rf /out/lib/tcc
