#!/bin/bash

TEST_ID="06"
TEST_NAME="Dir difference"

CREATE=`pwd`/$1
DEPLOY=`pwd`/$2
TEST_TOOLS=$3

. ./test_lib.sh

############# Test specific code ############

setup () {
	mkdir $ORIGIN/a && \
	mkdir $TARGET/a && \
	echo "1" > $TARGET/a/1 && \
	chown -h :cdrom $TARGET/a && \
	chmod 707 $TARGET/a
}

check_results () {
	test -d           $ORIGIN/a        && \
	test -f           $ORIGIN/a/1      && \
	check_same_mode   $ORIGIN/a $TARGET/a #&& \
	check_same_uidgid $ORIGIN/a $TARGET/a #&& \
	check_same_mode   $ORIGIN/a/1 $TARGET/a/1 && \
	check_same_uidgid $ORIGIN/a/1 $TARGET/a/1 && \
	check_content     $ORIGIN/a/1 "1"
}

#############################################
main $@
