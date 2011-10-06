#!/bin/bash

TEST_ID="09"
TEST_NAME="Character device add and remove"

CREATE=`pwd`/$1
DEPLOY=`pwd`/$2
TEST_TOOLS=$3

. ./test_lib.sh

############# Test specific code ############

setup () {
	#make a node with the same numbers as /dev/null
	setup_block_major_minor="`stat /dev/null -c '%t %T'`"
	mknod $ORIGIN/toremove c $setup_block_major_minor
	mknod $TARGET/toadd c $setup_block_major_minor
}

check_results () {
	test   -c $ORIGIN/toadd &&
	test ! -c $ORIGIN/toremove
}

#############################################
main $@
