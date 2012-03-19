.PHONY: all

CC := gcc

OPT ?= -O2

DESTDIR = /
PREFIX  = /usr

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
	install -m 755 -d $(DESTDIR)$(PREFIX)/bin
	install $(CREATE) $(DESTDIR)$(PREFIX)/bin
	install $(DEPLOY) $(DESTDIR)$(PREFIX)/bin

uninstall:
	rm -rf $(DESTDIR)/$(DEPLOY)
	rm -rf $(DESTDIR)/$(CREATE)

test:
	cd tests && ./run_tests.sh && fakeroot -- ./cross_plat.sh && cd ..

.PHONY: clean

check:
	cd tests && ./run_tests.sh && cd ..

clean:
	rm  -f $(DEPLOY) $(CREATE) *.o *.d
