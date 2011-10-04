#!/bin/bash

TEST_ID="05"
TEST_NAME="Symlink add/remove"

CREATE=`pwd`/$1
DEPLOY=`pwd`/$2
TEST_TOOLS=$3

. ./test_lib.sh

############# Test specific code ############

function setup {
	mkdir $ORIGIN/remove && \
	mkdir $TARGET/add && \
	chown -h :cdrom $TARGET/add
}

function check_results {
	test   -d $ORIGIN/add    && \
	test ! -d $ORIGIN/remove && \
	check_same_mtime  $ORIGIN/add $TARGET/add && \
	check_same_mode   $ORIGIN/add $TARGET/add && \
	check_same_uidgid $ORIGIN/add $TARGET/add
}

#############################################
main $@
