.PHONY: all
all: otap_deploy otap_create

otap_deploy: deploy.c otap*.c stat.c
	gcc -Wall -Wextra -O2 deploy.c otap*.c stat.c -o otap_deploy

otap_create: create.c otap*.c stat.c
	gcc -Wall -Wextra -O2 create.c otap*.c stat.c -o otap_create

.PHONY: clean
clean:
	rm  -f otap_deploy otap_create
