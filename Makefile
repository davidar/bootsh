MUSL = musl-1.2.5

.PHONY: all clean test

all: bootsh

clean:
	rm -f bootsh
	$(MAKE) -C lib/tcc clean
	rm -rf build
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

build/libtcc1a.h: lib/tcc/libtcc1.a
	mkdir -p build
	echo "#define LIBTCC1A_LEN $$(wc -c < $<)" > $@
	gzip -9 < $< | od -Anone -vtx1 | \
		sed 's/ /,0x/g;1s/^,/static char libtcc1a_data[] = {\n /;$$s/.*/&};/' >> $@

build/ash: FORCE lib/tcc/libtcc.a build/libtcc1a.h
	cd src && ninja

bootsh: build/ash
	cp -f $< $@

FORCE:

-include Makefile.docker
