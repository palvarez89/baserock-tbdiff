#!/bin/bash

TEST_ID="06"
TEST_NAME="Dir difference"

CREATE=`pwd`/$1
DEPLOY=`pwd`/$2
TEST_TOOLS=$3

. ./test_lib.sh

############# Test specific code ############

setup_origin () {
	mkdir $ORIGIN/a $ORIGIN/sticky $ORIGIN/setgid
}

setup_target () {
	mkdir $TARGET/a $TARGET/sticky $TARGET/setgid &&
	echo "1" > $TARGET/a/1 &&
	chown -h :cdrom $TARGET/a &&
	chmod +t $TARGET/sticky &&
	chmod g+s $TARGET/setgid &&
	chmod 707 $TARGET/a
}

check_results () {
	test -d           $ORIGIN/a        &&
	test -f           $ORIGIN/a/1      &&
	test -k           $ORIGIN/sticky   &&
	test -g           $ORIGIN/setgid   &&
	check_same_mode   $ORIGIN/a $TARGET/a && \
	check_same_uidgid $ORIGIN/a $TARGET/a && \
	check_same_mode   $ORIGIN/a/1 $TARGET/a/1 &&
	check_same_uidgid $ORIGIN/a/1 $TARGET/a/1 &&
	check_content     $ORIGIN/a/1 "1"
}

#############################################
main $@
