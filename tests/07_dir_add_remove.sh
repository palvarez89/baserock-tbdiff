#!/bin/bash

TEST_ID="07"
TEST_NAME="Directory add remove"

CREATE=`pwd`/$1
DEPLOY=`pwd`/$2
TEST_TOOLS=$3

. ./test_lib.sh

############# Test specific code ############

setup () {
	mkdir -p $ORIGIN/remove/1/2/3/4 && \
	mkdir -p $TARGET/add/4/3/2/1 && \
	chown -h :cdrom $TARGET/add
	chown -h :cdrom $TARGET/add/4/3/2/1
}

check_results () {
	test   -d $ORIGIN/add/4/3/2/1    && \
	test ! -d $ORIGIN/remove && \
	check_same_mtime  $ORIGIN/add $TARGET/add && \
	check_same_mode   $ORIGIN/add $TARGET/add && \
	check_same_uidgid $ORIGIN/add $TARGET/add && \
	check_same_mode   $ORIGIN/add $TARGET/add && \
	check_same_uidgid $ORIGIN/add/4/3/2/1 $TARGET/add/4/3/2/1
}

#############################################
main $@
