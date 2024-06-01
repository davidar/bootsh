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

build/ash: FORCE
	cd src && ninja

bootsh: build/ash
	cp -f $< $@

FORCE:

-include Makefile.docker
