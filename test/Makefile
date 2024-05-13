all:
	docker build .. -t bootsh
	docker run --rm \
		-v $(CURDIR)/../test/cc:/root/test/cc \
		-v $(CURDIR)/../lib/toybox:/root/lib/toybox \
		bootsh test-host

bootstrap:
	docker build .. -t bootsh
	docker build .. -f Dockerfile.bootstrap

sabotage: FORCE
	docker build .. -t bootsh
	docker run --rm -it bootsh sabotage

run:
	docker build .. -t bootsh
	docker run --rm -it bootsh

FORCE:
