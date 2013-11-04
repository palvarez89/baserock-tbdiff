#!/bin/bash

TEST_ID="07"
TEST_NAME="Directory add remove"

CREATE=`pwd`/$1
DEPLOY=`pwd`/$2
TEST_TOOLS=$3

. ./test_lib.sh

############# Test specific code ############

setup_origin () {
	mkdir -p $ORIGIN/remove/1/2/3/4
}

setup_target () {
	mkdir -p $TARGET/add/4/3/2/1 &&
	mkdir -p $TARGET/addsticky &&
	mkdir -p $TARGET/addsetgid &&
	chown -h :cdrom $TARGET/add &&
	chown -h :cdrom $TARGET/add/4/3/2/1 &&
	chmod +t $TARGET/addsticky &&
	chmod g+s $TARGET/addsetgid
}

check_results () {
	test   -d $ORIGIN/add/4/3/2/1 &&
	test ! -d $ORIGIN/remove &&
	test   -k $ORIGIN/addsticky &&
	test   -g $ORIGIN/addsetgid &&
	check_same_mtime  $ORIGIN/add $TARGET/add &&
	check_same_mode   $ORIGIN/add $TARGET/add &&
	check_same_uidgid $ORIGIN/add $TARGET/add &&
	check_same_mode   $ORIGIN/add $TARGET/add &&
	check_same_uidgid $ORIGIN/add/4/3/2/1 $TARGET/add/4/3/2/1
}

#############################################
main $@
