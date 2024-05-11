bootstrap:
	docker build .. -t bootsh
	docker build .. -f Dockerfile.bootstrap

sabotage: FORCE
	docker build .. -t bootsh
	docker build .. -f Dockerfile.sabotage -t sabotage
	docker run --rm -it sabotage /bin/sh

FORCE:
