all: build-seed bootstrap-0 build-bootstrap-0 bootstrap-1 build-bootstrap-1 bootstrap-2 bootgcc
# all: build-seed run

build-%:
	docker build .. -f Dockerfile -t bootsh-test --progress=plain --build-arg ROOTFS=rootfs/$* --target bootstrap

bootgcc:
	docker build .. -f Dockerfile -t bootgcc --progress=plain --build-arg ROOTFS=rootfs/bootstrap-2 --target gcc
	docker run --rm -it bootgcc /bin/bash

bootllvm:
	docker build .. -f Dockerfile -t bootllvm --progress=plain --build-arg ROOTFS=rootfs/bootstrap-2 --target llvm
	docker run --rm -it bootllvm /bin/sh

bootstrap-%:
	rm -rf ../rootfs/bootstrap-$*
	mkdir -p ../rootfs/bootstrap-$*
	ID=$$(docker create bootsh-test) && docker export $$ID | tar -C ../rootfs/bootstrap-$* --strip-components=1 -x out && docker rm $$ID

run:
	docker run --name bootsh-test --rm -it -v $(CURDIR)/..:/src/bootsh -w /src/bootsh bootsh-test

hello:
	docker run --rm bootsh-test /usr/bin/hello

debug:
	docker run -it \
		--pid=container:bootsh-test \
		--net=container:bootsh-test \
		--cap-add sys_admin \
		--cap-add sys_ptrace \
		invisiblethreat/docker-debug:latest
	# strace -p1 2>&1 | less
