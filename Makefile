SEEDFS = $(CURDIR)/rootfs/seed

.PHONY: seed test

seed:
	rm -rf $(SEEDFS)
	cd lib/tcc && ./configure && $(MAKE) && $(MAKE) DESTDIR=$(SEEDFS) install
	cd lib/musl && ./configure --target=x86_64 \
			CC=$(SEEDFS)/usr/local/bin/tcc \
			AR="$(SEEDFS)/usr/local/bin/tcc -ar" \
			RANLIB=echo \
			LIBCC=$(SEEDFS)/usr/local/lib/tcc/libtcc1.a && \
		$(MAKE) CFLAGS=-g && $(MAKE) DESTDIR=$(SEEDFS) install
	echo "GROUP ( $(SEEDFS)/usr/local/musl/lib/libc.a $(SEEDFS)/usr/local/lib/tcc/libtcc1.a )" > $(SEEDFS)/libc.ld
	cd lib/tcc && make clean && ./configure \
			--extra-cflags="-nostdinc -nostdlib -I$(SEEDFS)/usr/local/musl/include -DCONFIG_TCC_STATIC" \
			--extra-ldflags="-nostdlib $(SEEDFS)/usr/local/musl/lib/crt1.o $(SEEDFS)/libc.ld -static" \
			--config-ldl=no --config-debug=yes --config-bcheck=no && \
		$(MAKE) && $(MAKE) DESTDIR=$(SEEDFS) install
	cd lib/toybox && ln -sf ../../toybox.config .config && $(MAKE) \
			NOSTRIP=1 \
			CC=$(SEEDFS)/usr/local/bin/tcc \
			CFLAGS="-nostdinc -nostdlib -I$(SEEDFS)/usr/local/musl/include -I/usr/include -I/usr/include/x86_64-linux-gnu -g" \
			LDFLAGS="-nostdlib $(SEEDFS)/usr/local/musl/lib/crt1.o $(SEEDFS)/libc.ld -static"
	cd src && CC=$(SEEDFS)/usr/local/bin/tcc \
			CFLAGS="-nostdinc -I$(SEEDFS)/usr/local/musl/include" \
			LDFLAGS="-nostdlib -static" \
			LIBS="$(SEEDFS)/usr/local/musl/lib/crt1.o $(SEEDFS)/libc.ld" \
			$(MAKE)
	mkdir -p $(SEEDFS)/bin
	cp src/dash $(SEEDFS)/bin/sh

test:
	$(MAKE) -C test
