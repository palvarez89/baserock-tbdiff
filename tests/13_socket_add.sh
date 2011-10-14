#!/bin/bash

TEST_ID="13"
TEST_NAME="Socket device addition"

CREATE=`pwd`/$1
DEPLOY=`pwd`/$2
TEST_TOOLS=$3

. ./test_lib.sh

############# Test specific code ############
# sockets can't be added sensibly, test that it hasn't been
SOCKBIND=`mktemp`
setup_origin () {
	true
}

setup_target () {
	gcc sockbind.c -o $SOCKBIND 2>/dev/null >/dev/null
	$SOCKBIND "$TARGET/toadd" &
	SOCKBINDPID=$!
	until test -S "$TARGET/toadd"; do :; done
	kill $SOCKBINDPID && wait $SOCKBINDPID 2>/dev/null
	rm -f $SOCKBIND
}

#tbdiff-create should fail when it would have to add a socket
create_test_return () {
	test "$1" != "0" 
}

check_results () {
	false #test should never reach this
}

#############################################
main $@
