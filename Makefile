.POSIX:
.PHONY: all clean install

all: src/samu/samu
	$<

src/samu/samu:
	$(MAKE) -C src/samu

clean:
	rm -rf build lib/toybox/generated

install: all
	./install.sh
