all: build run

build:
	docker build .. -f Dockerfile -t bootsh-test --progress=plain

rootfs:
	ID=$$(docker create bootsh-test) && rm -rf rootfs && mkdir -p rootfs && \
		docker export $$ID | tar -x -C rootfs && docker rm $$ID

run:
	docker run --name bootsh-test --rm -it bootsh-test

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
