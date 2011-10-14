#!/bin/bash

TEST_ID="14"
TEST_NAME="Socket device difference"

CREATE=`pwd`/$1
DEPLOY=`pwd`/$2
TEST_TOOLS=$3

. ./test_lib.sh

############# Test specific code ############
# sockets can't be changed sensibly, test that it hasn't been

SOCKBIND=`mktemp`
setup_origin () {
	gcc sockbind.c -o $SOCKBIND 2>/dev/null >/dev/null
	$SOCKBIND "$ORIGIN/tochange" &
	SOCKBINDPID=$!
	until test -S "$ORIGIN/tochange"; do :; done
	kill $SOCKBINDPID
	wait $SOCKBINDPID 2>/dev/null || true #wait returns false
}

setup_target () {
	$SOCKBIND "$TARGET/tochange" &
	SOCKBINDPID=$!
	until test -S "$TARGET/tochange"; do :; done
	kill $SOCKBINDPID &&
	wait $SOCKBINDPID 2>/dev/null #surpress terminated output
	rm -f $SOCKBIND
}

#tbdiff-create should fail to make a patch if it would have to change a socket 
create_test_return () {
	test "$1" != 0	
}

check_results () {
	false #test should never reach this
}

#############################################
main $@
