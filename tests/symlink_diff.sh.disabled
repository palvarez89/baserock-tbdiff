#!/bin/bash

TEST_ID="04"
TEST_NAME="Symlink diff"

CREATE=`pwd`/$1
DEPLOY=`pwd`/$2
TEST_TOOLS=$3

. ./test_lib.sh

############# Test specific code ############

setup_origin () {
	(
		cd $ORIGIN &&
		echo 1 >file &&
		chown :cdrom file &&
		mkdir -p dir &&
		chown :cdrom dir &&
		ln -s file flink &&
		ln -s dir dlink
		ln -s /foo a
	)
}

setup_target () {
	(
		cd $TARGET &&
		echo 1 >file &&
		chown :cdrom file &&
		mkdir -p dir &&
		chown :cdrom dir &&
		ln -s file flink &&
		ln -s dir dlink
		chgrp -h daemon flink dlink &&
		ln -s /bar a &&
		chown -h :cdrom a
	)
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
