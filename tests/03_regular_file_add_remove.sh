#!/bin/bash

TEST_ID="01"
TEST_NAME="Regular file add remove"

CREATE=`pwd`/$1
DEPLOY=`pwd`/$2
TEST_TOOLS=$3

. ./test_lib.sh

############# Test specific code ############

# This test checks that normal files content and metadata are 

function setup {
	touch $ORIGIN/remove && \
	echo 1 > $TARGET/add && \
	chown -h :cdrom $TARGET/add
}

# check_same_mtime FILE_A FILE_B

function check_results {
	test   -f $ORIGIN/add    && \
	test ! -f $ORIGIN/remove && \
	check_content     $ORIGIN/add "1"         && \
	check_same_mtime  $ORIGIN/add $TARGET/add && \
	check_same_uidgid $ORIGIN/add $TARGET/add
}

#############################################
main $@
