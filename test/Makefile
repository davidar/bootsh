all:
	docker build .. -f Dockerfile.bootstrap -t bootsh-test --progress=plain --build-arg BOOTSH=bootsh
	ID=$$(docker create bootsh-test) && docker cp $$ID:/build/bootsh/bootsh ../bootsh.0 && docker rm $$ID
	docker build .. -f Dockerfile.bootstrap -t bootsh-test --progress=plain --build-arg BOOTSH=bootsh.0
	ID=$$(docker create bootsh-test) && docker cp $$ID:/build/bootsh/bootsh ../bootsh.1 && docker rm $$ID
	docker build .. -f Dockerfile.bootstrap -t bootsh-test --progress=plain --build-arg BOOTSH=bootsh.1
	ID=$$(docker create bootsh-test) && docker cp $$ID:/build/bootsh/bootsh ../bootsh.2 && docker rm $$ID

sabotage-stage0: FORCE
	docker build .. -f Dockerfile.sabotage -t sabotage-stage0 --progress=plain --build-arg BOOTSH=bootsh --target stage0
	rm -rf sabotage-stage0
	mkdir -p sabotage-stage0
	ID=$$(docker create sabotage-stage0) && docker export $$ID | tar -C sabotage-stage0 --strip-components=1 -x sabotage && docker rm $$ID

sabotage-stage1:
	docker build .. -f Dockerfile.sabotage -t sabotage-stage1 --progress=plain --target stage1
	docker run --rm -it sabotage-stage1 /bin/sh

run:
	docker run --name bootsh-test --rm -it -v $(CURDIR)/..:/build/bootsh -w /build/bootsh bootsh-test

debug:
	docker run -it \
		--pid=container:bootsh-test \
		--net=container:bootsh-test \
		--cap-add sys_admin \
		--cap-add sys_ptrace \
		invisiblethreat/docker-debug:latest
	# strace -p1 2>&1 | less

FORCE:
