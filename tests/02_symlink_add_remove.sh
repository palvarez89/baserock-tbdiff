#!/bin/bash

TEST_ID="03"
TEST_NAME="Symlink add/remove"

CREATE=`pwd`/$1
DEPLOY=`pwd`/$2
TEST_TOOLS=$3

. ./test_lib.sh

############# Test specific code ############

function setup {
	ln -s /foo $ORIGIN/remove && \
	ln -s /bar $TARGET/add && \
	chown -h :cdrom $TARGET/add
}

function check_results {
	test   -L $ORIGIN/add    && \
	test ! -L $ORIGIN/remove && \
	check_symlink     $ORIGIN/add "/bar"      && \
	check_same_mtime  $ORIGIN/add $TARGET/add && \
	check_same_uidgid $ORIGIN/add $TARGET/add
}

#############################################
main $@
