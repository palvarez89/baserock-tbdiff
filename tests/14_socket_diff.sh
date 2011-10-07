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
setup () {
	gcc sockbind.c -o $SOCKBIND 2>/dev/null >/dev/null
	$SOCKBIND "$ORIGIN/tochange" &
	SOCKBINDPID1=$!
	$SOCKBIND "$TARGET/tochange" &
	SOCKBINDPID2=$!
	until test -S "$TARGET/tochange" -a -S "$ORIGIN/tochange"; do :; done
	kill $SOCKBINDPID1 $SOCKBINDPID2 &&
	wait $SOCKBINDPID1 $SOCKBINDPID2 2>/dev/null #surpress terminated output
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
