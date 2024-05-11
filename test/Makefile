all:
	docker build .. -f Dockerfile.bootstrap -t bootsh-test --progress=plain --build-arg BOOTSH=bootsh
	ID=$$(docker create bootsh-test) && docker cp $$ID:/build/bootsh/bootsh ../bootsh.0 && docker rm $$ID
	docker build .. -f Dockerfile.bootstrap -t bootsh-test --progress=plain --build-arg BOOTSH=bootsh.0
	ID=$$(docker create bootsh-test) && docker cp $$ID:/build/bootsh/bootsh ../bootsh.1 && docker rm $$ID
	docker build .. -f Dockerfile.bootstrap -t bootsh-test --progress=plain --build-arg BOOTSH=bootsh.1
	ID=$$(docker create bootsh-test) && docker cp $$ID:/build/bootsh/bootsh ../bootsh.2 && docker rm $$ID

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

sabotage: FORCE
	docker build .. -f Dockerfile.sabotage -t sabotage --progress=plain
	docker run --rm -it sabotage /bin/sh

FORCE:
