#!/bin/bash

TEST_ID="08"
TEST_NAME="Character device difference"

CREATE=`pwd`/$1
DEPLOY=`pwd`/$2
TEST_TOOLS=$3

. ./test_lib.sh

############# Test specific code ############

setup () {
	mknod $ORIGIN/tochange c `stat /dev/null -c '%t %T'`
	mknod $TARGET/tochange c `stat /dev/full -c '%t %T'` 
}

check_results () {
	test -c $ORIGIN/tochange &&
	test "`stat -c '%t %T' $ORIGIN/tochange`" = \
	     "`stat -c '%t %T' $TARGET/tochange`"
}

#############################################
main $@
