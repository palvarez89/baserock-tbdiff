#!/bin/bash

TEST_ID="11"
TEST_NAME="Block device add and remove"

CREATE=`pwd`/$1
DEPLOY=`pwd`/$2
TEST_TOOLS=$3

. ./test_lib.sh

############# Test specific code ############

setup () {
	#make a node with the same numbers as /dev/null
	setup_block_major_minor="`stat /dev/null -c '%t %T'`"
	mknod $ORIGIN/toremove b $setup_block_major_minor
	mknod $TARGET/toadd b $setup_block_major_minor
}

check_results () {
	test   -b $ORIGIN/toadd &&
	test ! -b $ORIGIN/toremove
}

#############################################
main $@
