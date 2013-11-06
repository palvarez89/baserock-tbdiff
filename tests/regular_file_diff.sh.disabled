#!/bin/bash

TEST_ID="00"
TEST_NAME="Simple file diff"

CREATE=`pwd`/$1
DEPLOY=`pwd`/$2
TEST_TOOLS=$3

. ./test_lib.sh

############# Test specific code ############

ORG_FILE=$ORIGIN/b.txt
TGT_FILE=$TARGET/b.txt

setup_origin () {
	echo 1 >$ORIGIN/a.txt   &&
	chgrp tty $ORIGIN/a.txt &&
	echo 1 > $ORG_FILE
}

setup_target () {
	echo 2 >$TARGET/a.txt   &&
	chgrp tty $TARGET/a.txt &&
	echo 2 > $TGT_FILE      &&
	chown :cdrom $TGT_FILE  &&
	chmod 707 $TGT_FILE
}

check_results () {
	check_group      $ORIGIN/a.txt tty &&
	check_content    $ORG_FILE "2" && \
	check_perm       $ORG_FILE 707 && \
	check_group      $ORG_FILE cdrom && \
	check_same_mtime $ORG_FILE $TGT_FILE
}

#############################################
main $@
