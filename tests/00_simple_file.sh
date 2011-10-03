#!/bin/bash

TEST_ID="00"
TEST_NAME="Simple file diff"

CREATE=`pwd`/$1
DEPLOY=`pwd`/$2
TEST_TOOLS=$3

. ./test_lib.sh

function setup {
	rm -rf $TESTDIR
	mkdir -p $ORIGIN       && \
	mkdir -p $TARGET       && \
	echo 1 > $ORG_FILE     && \
	echo 2 > $TGT_FILE     && \
	chgrp cdrom $TGT_FILE    && \
	chmod 707 $TGT_FILE
}

# check_same_mtime FILE_A FILE_B

function check_results {
	check_content    $ORG_FILE "2" && \
	check_perm       $ORG_FILE 707 && \
	check_same_mtime $ORG_FILE $TGT_FILE
}

main $@
