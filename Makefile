MUSL = musl-1.2.5

.PHONY: all seed test diff bootstrap

all: bootsh

clean:
	rm -f bootsh
	$(MAKE) -C lib/tcc clean
	$(MAKE) -C lib/musl clean
	$(MAKE) -C lib/toybox clean
	$(MAKE) -C src clean

musl-%.tar.gz:
	wget https://musl.libc.org/releases/$@

musl-%: musl-%.tar.gz
	tar xf $<

sysroot: $(MUSL)
	cd $(MUSL) && ./configure --prefix="$(CURDIR)/sysroot"
	$(MAKE) -C $(MUSL) CFLAGS="-Os"
	$(MAKE) -j1 -C $(MUSL) install

lib/tcc/libtcc.a: FORCE
	$(MAKE) -C lib/tcc libtcc.a

lib/tcc/libtcc1.a: FORCE lib/tcc/libtcc.a
	$(MAKE) -C lib/tcc libtcc1.a

src/libtcc1a.h: lib/tcc/libtcc1.a
	echo "#define LIBTCC1A_LEN $$(wc -c < $<)" > $@
	gzip -9 < $< | od -Anone -vtx1 | \
		sed 's/ /,0x/g;1s/^,/static char libtcc1a_data[] = {\n /;$$s/.*/&};/' >> $@

lib/toybox/libtoybox.a: FORCE
	$(MAKE) -C lib/toybox libtoybox.a NOSTRIP=1

src/dash: FORCE lib/tcc/libtcc.a src/libtcc1a.h lib/toybox/libtoybox.a
	$(MAKE) -C src dash

bootsh: src/dash
	cp -f $< $@


SEED = $(CURDIR)/rootfs/seed

seed: bootsh
	rm -rf $(SEED)
	mkdir -p $(SEED)/bin
	cp bootsh $(SEED)/bin/sh
	strip $(SEED)/bin/sh

test: seed
	$(MAKE) -C test

diff:
	diffoscope --exclude-directory-metadata=yes rootfs/bootstrap-1 rootfs/bootstrap-2

bootstrap:
	cd lib/tcc && $(MAKE) clean && ./configure --cc=cc --ar=ar --prefix=/ \
		--config-ldl=no --config-debug=yes --config-bcheck=no --config-backtrace=no && \
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

FORCE:
