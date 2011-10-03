#!/bin/bash

TEST_ID="01"
TEST_NAME="Simple symlink diff"

CREATE=`pwd`/$1
DEPLOY=`pwd`/$2
TEST_TOOLS=$3

. ./test_lib.sh

############# Test specific code ############

# This test checks that normal files content and metadata are 

function setup {
	ln -s /foo $ORIGIN/a && \
	ln -s /bar $TARGET/a && \
	chown -h :cdrom $TARGET/a
}

# check_same_mtime FILE_A FILE_B

function check_results {
	test -L $ORIGIN/a           && \
	check_symlink    $ORIGIN/a "/bar"        && \
	check_group      $ORIGIN/a cdrom         && \
	check_same_mtime $ORIGIN/a $ORIGIN/a
}

#############################################
main $@
