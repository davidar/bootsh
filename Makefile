MUSL = musl-1.2.5

.PHONY: all clean test

all: bootsh

clean:
	rm -f bootsh
	$(MAKE) -C lib/tcc clean
	$(MAKE) -C lib/toybox clean
	$(MAKE) -C lib/sbase clean
	$(MAKE) -C src clean
	if [ -d test-cc ]; then $(MAKE) -C test-cc clean; fi

tarballs/musl-%.tar.gz:
	mkdir -p tarballs
	wget https://musl.libc.org/releases/musl-$*.tar.gz -O $@

musl-%: tarballs/musl-%.tar.gz
	tar -xf $<

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

lib/sbase/%: FORCE
	$(MAKE) -C lib/sbase $*

src/ash: FORCE lib/tcc/libtcc.a src/libtcc1a.h lib/toybox/libtoybox.a lib/sbase/getconf.h lib/sbase/libutf.a lib/sbase/libutil.a
	$(MAKE) -C src ash

bootsh: src/ash
	cp -f $< $@

FORCE:

-include Makefile.docker
