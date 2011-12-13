.PHONY: all

CC := gcc

OPT ?= -O2

DEPLOY=tbdiff-deploy
CREATE=tbdiff-create

CFLAGS ?=
CFLAGS += -g
CFLAGS += -Wall -Wextra -Werror -Wno-unused-result $(OPT)

SHARED_SRC := libtbd_stat.c libtbd_xattrs.c libtbd_io.c
DEPLOY_SRC := tbdiff_deploy.c libtbd_create.c
CREATE_SRC := tbdiff_create.c libtbd_apply.c

DEPLOY_OBJ := $(patsubst %.c,%.o,$(SHARED_SRC) $(DEPLOY_SRC))
CREATE_OBJ := $(patsubst %.c,%.o,$(SHARED_SRC) $(CREATE_SRC))

all: $(DEPLOY) $(CREATE)

$(DEPLOY): tbdiff_deploy.o libtbd_apply.o libtbd_stat.o libtbd_xattrs.o libtbd_io.o
	$(CC) $(LDFLAGS) -o $@ $^

$(CREATE): tbdiff_create.o libtbd_create.o libtbd_stat.o libtbd_xattrs.o libtbd_io.o
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

test:
	cd tests && ./run_tests.sh && cd ..
.PHONY: clean
clean:
	rm  -f $(DEPLOY) $(CREATE) *.o *.d
check: test
