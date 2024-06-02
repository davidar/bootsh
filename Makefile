all: src/samu/samu
	$<

src/samu/samu:
	$(MAKE) -C src/samu

clean:
	rm -rf build

install: all
	./install.sh
