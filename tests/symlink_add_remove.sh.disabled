#!/bin/bash

TEST_ID="05"
TEST_NAME="Symlink add/remove"

CREATE=`pwd`/$1
DEPLOY=`pwd`/$2
TEST_TOOLS=$3

. ./test_lib.sh

############# Test specific code ############

setup_origin () {
	(
		cd $ORIGIN
		ln -s /foo remove &&
		mkdir -p data &&
		touch data/a data/b && 
		ln -s data datalink
	)
}

setup_target () {
	mkdir -p $TARGET/data &&
	touch $TARGET/data/a $TARGET/data/b &&
	ln -s /bar $TARGET/add &&
	chown -h :cdrom $TARGET/add
}

check_results () {
	test   -L $ORIGIN/add    &&
	test ! -L $ORIGIN/remove &&
	check_symlink     $ORIGIN/add "/bar"      &&
	check_same_mtime  $ORIGIN/add $TARGET/add &&
	check_same_uidgid $ORIGIN/add $TARGET/add &&
	test ! -L $ORIGIN/datalink &&
	echo datalink not link &&
	test   -d $ORIGIN/data &&
	echo data is dir &&
	test   -f $ORIGIN/data/a &&
	echo a is dir &&
	test   -f $ORIGIN/data/b &&
	echo b is dir
}

#############################################
main $@
