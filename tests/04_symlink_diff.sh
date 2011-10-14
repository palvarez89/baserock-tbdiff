#!/bin/bash

TEST_ID="04"
TEST_NAME="Symlink diff"

CREATE=`pwd`/$1
DEPLOY=`pwd`/$2
TEST_TOOLS=$3

. ./test_lib.sh

############# Test specific code ############

setup () {
	for dir in $ORIGIN $TARGET; do
	(
		cd $dir &&
		echo 1 >file &&
		chown :cdrom file &&
		mkdir -p dir &&
		chown :cdrom dir &&
		ln -s file flink &&
		ln -s dir dlink
	); done &&
	chgrp -h daemon $TARGET/flink $TARGET/dlink &&
	ln -s /foo $ORIGIN/a && \
	ln -s /bar $TARGET/a && \
	chown -h :cdrom $TARGET/a
}

check_results () {
	test -f           $ORIGIN/file &&
	check_group       $ORIGIN/file cdrom &&
	test -d           $ORIGIN/dir &&
	check_group       $ORIGIN/dir cdrom &&
	check_group       $ORIGIN/flink daemon &&
	check_group       $ORIGIN/dlink daemon &&
	test -L           $ORIGIN/a && \
	check_symlink     $ORIGIN/a "/bar" && \
	check_group       $ORIGIN/a cdrom     && \
	check_same_mtime  $ORIGIN/a $TARGET/a && \
	check_same_uidgid $ORIGIN/a $TARGET/a
}

#############################################
main $@
