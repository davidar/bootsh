bootstrap:
	docker build .. -t bootsh
	docker build .. -f Dockerfile.bootstrap

sabotage: FORCE
	docker build .. -t bootsh
	docker build .. -f Dockerfile.sabotage -t sabotage
	docker run --rm -it sabotage /bin/sh

run:
	docker build .. -t bootsh
	docker run --rm -it bootsh /bin/sh

FORCE:
