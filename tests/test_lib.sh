OK=" OK"
FAIL=" FAIL"

TESTDIR=/tmp/tbdiff-test-`uuid`
IMGFILE=$TESTDIR/tbdiff.img
ORIGIN=$TESTDIR/orig
TARGET=$TESTDIR/target

ORG_FILE=$ORIGIN/b.txt
TGT_FILE=$TARGET/b.txt

function check_same_mtime {
	test $(stat -c %Y $1) = $(stat -c %Y $2)
}

# check_content FILE EXPECTED_OCTAL_PERMISSIONS
function check_perm {
	test $(stat -c %a $1) = $2
}

# check_content FILE EXPECTED_CONTENT
function check_content {
	test $(cat $1) = $2
}

function start {
	if [ $# -ne 2 ]
	then
		echo "ERROR: Not enough arguments."
		cleanup_and_exit
	fi

	if [ ! -f $1 ]
	then
		echo "ERROR: $1 is an invalid tbdiff-create path"  1>&2
		cleanup_and_exit
	fi

	if [ ! -f $2 ]
	then
		echo "ERROR: $1 is an invalid tbdiff-deploy path" 1>&2
		cleanup_and_exit
	fi
}

function cleanup_and_exit {
	rm -rf $TESTDIR
	exit 1
}

function main {
	start $@
	echo -n "$TEST_ID Setting up $TEST_NAME test: "
	setup
	if [ $? -ne 0 ]
	then
		echo $FAIL
		echo "Couldn't setup the test directory structure. Check your privileges" 1>&2
		cleanup_and_exit
	fi
	echo $OK

	echo -n "$TEST_ID Performing $TEST_NAME image creation and deployment: "
	CWD=$(pwd)
	$CREATE $IMGFILE $ORIGIN $TARGET && \
	cd $ORIGIN && \
	$DEPLOY $IMGFILE && \
	RETVAL=$?
	cd $CWD
	
	if test "x$RETVAL" != "x0"
	then
		echo $FAIL
		echo "Could not create and deploy image." 1>&2
		cleanup_and_exit
	fi
	echo $OK

	echo -n "$TEST_ID Checking $TEST_NAME results: "
	check_results
	if test "x$RETVAL" != "x0"
	then
		echo $FAIL
		echo "Applying image did not produce the expected results" 1>&2
		cleanup_and_exit
	fi
	echo $OK
}
