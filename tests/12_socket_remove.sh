#!/bin/bash

TEST_ID="12"
TEST_NAME="Socket device removal"

CREATE=`pwd`/$1
DEPLOY=`pwd`/$2
TEST_TOOLS=$3

. ./test_lib.sh

############# Test specific code ############
# sockets can only be moved, linked or removed by the file system
# creation is only performed by the program that acts as the server
# tbdiff won't be doing that so the only sensible operation is removal
SOCKBIND=`mktemp`
setup () {
	# sockbind creates a socket then writes any data written to it to stdout
	# have to fork it because it will never stop, have to wait for it to
	# make the socket
	gcc sockbind.c -o $SOCKBIND 2>/dev/null >/dev/null
	$SOCKBIND "$ORIGIN/toremove" &
	SOCKBINDPID=$!
	until test -S "$ORIGIN/toremove"; do :; done
	kill $SOCKBINDPID && wait $SOCKBINDPID 2>/dev/null
	rm -f $SOCKBIND
}

create_test_return () {
	test $1 = 0
}

check_results () {
	test ! -S "$ORIGIN/toremove"
}

#############################################
main $@
