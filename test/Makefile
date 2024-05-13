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
