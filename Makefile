.PHONY: all

CC := gcc

OPT ?= -O2

DEPLOY=tbdiff-deploy
CREATE=tbdiff-create

CFLAGS ?=
CFLAGS += -g
CFLAGS += -Wall -Wextra -Werror $(OPT)

SHARED_SRC := stat.c
DEPLOY_SRC := deploy.c otap_apply.c
CREATE_SRC := create.c otap_create.c

DEPLOY_OBJ := $(patsubst %.c,%.o,$(SHARED_SRC) $(DEPLOY_SRC))
CREATE_OBJ := $(patsubst %.c,%.o,$(SHARED_SRC) $(CREATE_SRC))



all: $(DEPLOY) $(CREATE)

$(DEPLOY): deploy.o otap_apply.o stat.o
	$(CC) $(LDFLAGS) -o $@ $^

$(CREATE): create.o otap_create.o stat.o
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -MMD -o $@ -c $<

ifeq (,$(findstring clean,$(MAKECMDGOALS)))
-include $(patsubst %.c,%.d,$(SHARED_SRC) $(DEPLOY_SRC) $(CREATE_SRC))
endif

install: $(DEPLOY) $(CREATE)
	install $(CREATE) /usr/local/bin
	install $(DEPLOY) /usr/local/bin

uninstall:
	rm -rf /usr/local/bin/$(DEPLOY)
	rm -rf /usr/local/bin/$(CREATE)

.PHONY: clean
clean:
	rm  -f $(DEPLOY) $(CREATE) *.o *.d
