all:
	docker build .. -f Dockerfile -t bootsh-test --progress=plain --build-arg BOOTSH=bootsh --target bootstrap
	ID=$$(docker create bootsh-test) && docker cp $$ID:/src/bootsh/bootsh ../bootsh.0 && docker rm $$ID
	docker build .. -f Dockerfile -t bootsh-test --progress=plain --build-arg BOOTSH=bootsh.0 --target bootstrap
	ID=$$(docker create bootsh-test) && docker cp $$ID:/src/bootsh/bootsh ../bootsh.1 && docker rm $$ID
	docker build .. -f Dockerfile -t bootsh-test --progress=plain --build-arg BOOTSH=bootsh.1 --target bootstrap
	ID=$$(docker create bootsh-test) && docker cp $$ID:/src/bootsh/bootsh ../bootsh.2 && docker rm $$ID

bootgcc:
	docker build .. -f Dockerfile -t bootgcc --progress=plain --build-arg BOOTSH=bootsh.2 --target gcc
	docker run --rm -it bootgcc /bin/bash

sabotage-stage0: FORCE
	docker build .. -f Dockerfile -t sabotage-stage0 --progress=plain --build-arg BOOTSH=bootsh --target sabotage-stage0
	# docker run --rm -it -v $(CURDIR)/sabotage:/src/sabotage sabotage-stage0 /bin/bash
	# rm -rf sabotage-stage0
	# mkdir -p sabotage-stage0
	# ID=$$(docker create sabotage-stage0) && docker export $$ID | tar -C sabotage-stage0 --strip-components=2 -x tmp/sabotage && docker rm $$ID

sabotage-stage1:
	docker build .. -f Dockerfile -t sabotage-stage1 --progress=plain --target sabotage-stage1
	docker run --rm -it sabotage-stage1 /bin/sh

bootllvm:
	docker build .. -f Dockerfile -t bootllvm --progress=plain --build-arg BOOTSH=bootsh.2 --target llvm
	docker run --rm -it bootllvm /bin/sh

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

FORCE:
