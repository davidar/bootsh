.POSIX:
.PHONY: all clean install

all: src/samu/samu
	$<

src/samu/samu:
	$(MAKE) -C src/samu

clean:
	rm -rf build lib/toybox/generated
	$(MAKE) -C src/samu clean

install: all
	scripts/install.sh
