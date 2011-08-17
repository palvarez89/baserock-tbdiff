.PHONY: all
all: otap_deploy otap_create

otap_deploy:
	gcc deploy.c otap*.c stat.c -o otap_deploy

otap_create:
	gcc create.c otap*.c stat.c -o otap_create

.PHONY: clean
clean:
	rm  -f otap_deploy otap_create
