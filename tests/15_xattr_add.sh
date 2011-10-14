#!/bin/bash

TEST_ID="15"
TEST_NAME="Extended Attributes manipulation"

CREATE=`pwd`/$1
DEPLOY=`pwd`/$2
TEST_TOOLS=$3

. ./test_lib.sh

############# Test specific code ############
if is_command getfattr && is_command setfattr; then :; else
	echo Test requires commands: getfattr, setfattr, attr >&2
	exit 127
fi

setup_origin () {
	touch $ORIGIN/file &&
	setfattr -n "user.preserve" -v "true" $ORIGIN/file &&
	setfattr -n "user.change" -v "false" $ORIGIN/file &&
	setfattr -n "user.remove" -v "false" $ORIGIN/file
}

setup_target () {
	touch $TARGET/file &&
	setfattr -n "user.preserve" -v "true" $TARGET/file &&
	setfattr -n "user.change" -v "true" $TARGET/file &&
	setfattr -n "user.add" -v "true" $TARGET/file
}

check_results () {
	check_xattrs $ORIGIN/file $TARGET/file
}

#############################################
main $@
