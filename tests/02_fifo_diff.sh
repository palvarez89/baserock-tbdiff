#!/bin/bash

TEST_ID="02"
TEST_NAME="Named pipe (FIFO) diff test"

CREATE=`pwd`/$1
DEPLOY=`pwd`/$2
TEST_TOOLS=$3

. ./test_lib.sh

############# Test specific code ############

setup_origin () {
	mkfifo $ORIGIN/remove
}

setup_target () {
	mkfifo $TARGET/add    &&
	chmod 707 $TARGET/add &&
	chown -h :cdrom $TARGET/add
}

check_results () {
	test   -p $ORIGIN/add    && \
	test ! -p $ORIGIN/remove && \
	check_same_mtime  $ORIGIN/add $TARGET/add && \
	check_same_uidgid $ORIGIN/add $TARGET/add
}

#############################################
main $@
