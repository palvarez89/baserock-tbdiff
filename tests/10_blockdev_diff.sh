#!/bin/bash

TEST_ID="10"
TEST_NAME="Block device difference"

CREATE=`pwd`/$1
DEPLOY=`pwd`/$2
TEST_TOOLS=$3

. ./test_lib.sh

############# Test specific code ############

setup_origin () {
	mknod $ORIGIN/tochange b `stat /dev/null -c '%t %T'`
}

setup_target () {
	mknod $TARGET/tochange b `stat /dev/full -c '%t %T'` 
}

check_results () {
	test -b $ORIGIN/tochange &&
	test "`stat -c '%t %T' $ORIGIN/tochange`" = \
	     "`stat -c '%t %T' $TARGET/tochange`"
}

#############################################
main $@
