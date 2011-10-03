#!/bin/bash

TEST_ID="01"
TEST_NAME="Symlink add/remove"

CREATE=`pwd`/$1
DEPLOY=`pwd`/$2
TEST_TOOLS=$3

. ./test_lib.sh

############# Test specific code ############

function setup {
	ln -s /foo $ORIGIN/a && \
	ln -s /bar $TARGET/a && \
	chown -h :cdrom $TARGET/a
}

function check_results {
	test -L           $ORIGIN/a && \
	check_symlink     $ORIGIN/a "/bar" && \
	check_group       $ORIGIN/a cdrom     && \
	check_same_mtime  $ORIGIN/a $TARGET/a && \
	check_same_uidgid $ORIGIN/a $TARGET/a
}

#############################################
main $@
