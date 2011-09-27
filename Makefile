.PHONY: all
all: otap_deploy otap_create

CC := gcc

OPT ?= -O2

CFLAGS ?=
CFLAGS += -g
CFLAGS += -Wall -Wextra -Werror $(OPT)

SHARED_SRC := otap.c stat.c
DEPLOY_SRC := deploy.c otap_apply.c
CREATE_SRC := create.c otap_create.c

DEPLOY_OBJ := $(patsubst %.c,%.o,$(SHARED_SRC) $(DEPLOY_SRC))
CREATE_OBJ := $(patsubst %.c,%.o,$(SHARED_SRC) $(CREATE_SRC))

otap_deploy: deploy.o otap_apply.o otap.o stat.o
	$(CC) $(LDFLAGS) -o $@ $^

otap_create: create.o otap_create.o otap.o stat.o
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -MMD -o $@ -c $<

ifeq (,$(findstring clean,$(MAKECMDGOALS)))
-include $(patsubst %.c,%.d,$(SHARED_SRC) $(DEPLOY_SRC) $(CREATE_SRC))
endif

install: otap_create otap_deploy
	install otap_create /usr/local/bin
	install otap_deploy /usr/local/bin

uninstall:
	rm -rf /usr/local/bin/otap_create
	rm -rf /usr/local/bin/otap_crate

.PHONY: clean
clean:
	rm  -f otap_deploy otap_create *.o *.d
