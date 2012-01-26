OK=" OK"
FAIL=" FAIL"

TESTDIR=`mktemp -d`
IMGFILE=$TESTDIR/tbdiff.img
ORIGIN=$TESTDIR/orig
TARGET=$TESTDIR/target

nums=( zero one two three four five six seven )
files=( null a.txt b.txt dirdir dirdir/fifo dirdir/symlinkb dirdir/chardev \
        dirdir/blockdev )

TOPDIR=`pwd`

insertfiles() {
    if [ $1 -ge 1 ]; then
        echo 1 > a.txt
    fi  
    if [ $1 -ge 2 ]; then 
        cat $TOPDIR/lipsum.txt > b.txt
    fi  
    if [ $1 -ge 3 ]; then 
        mkdir -p dirdir
    fi  
    if [ $1 -ge 4 ]; then 
        mkfifo dirdir/fifo
    fi  
    if [ $1 -ge 5 ]; then 
        ln -s ../b.txt dirdir/symlinkb
    fi  
    if [ $1 -ge 6 ]; then 
        mknod dirdir/chardev c `stat /dev/null -c '%t %T'`
    fi  
    if [ $1 -ge 7 ]; then 
        mknod dirdir/blockdev b `stat /dev/null -c '%t %T'`
    fi  
}

# check_same_mtime FILE_A FILE_B
check_same_mtime () {
	test $(stat -c %Y $1) = $(stat -c %Y $2)
}

# check_same_uidgid FILE_A FILE_B
check_same_uidgid () {
	test $(stat -c "%u.%g" $1) = $(stat -c "%u.%g" $2)
}

# check_same_mode FILE_A FILE_B
check_same_mode () {
	test $(stat -c "%f" $1) = $(stat -c "%f" $2)
}

# check_content FILE EXPECTED_OCTAL_PERMISSIONS
check_perm () {
	test $(stat -c %a $1) = $2
}

# check_content FILE EXPECTED_OCTAL_PERMISSIONS
check_symlink () {
	test $(readlink $1) = $2
}

# check_content FILE EXPECTED_CONTENT
check_content () {
	test $(cat $1) = $2
}

# check_group FILE EXPECTED_GROUP_NAME
check_group () {
	test $(stat -c %G $1) = $2
}

check_xattrs () {
	test "`getfattr -d $1 2>/dev/null | tail -n +2`" = \
	     "`getfattr -d $2 2>/dev/null | tail -n +2`"
}

# tests whether a command exists
is_command () {
	type $1 >/dev/null 2>/dev/null
}

#check_command COMMAND_STRING TEST_COMMAND COMMAND_DESCRIPTION
check_command () {
	COMMAND_STRING="$1"
	COMMAND_DESCRIPTION="$2"
	TEST_COMMAND="$3"
	eval $COMMAND_STRING
	RETVAL=$?
	if is_command "$TEST_COMMAND"; then #test explicitly checks return
		if $TEST_COMMAND $RETVAL; then
			if [ "$RETVAL" != "0" ]; then
				echo $COMMAND_STRING expected failure in \
				     $COMMAND_DESCRIPTION >&2
				echo $OK
				exit 0
			fi
		else
			if [ "$RETVAL" = "0" ]; then
				echo $COMMAND_STRING Unexpected success in \
				     $COMMAND_DESCRIPTION >&2
				echo $FAIL
				cleanup_and_exit
			else
				echo $COMMAND_STRING Unexpected failure in \
				     $COMMAND_DESCRIPTION >&2
				echo $FAIL
				cleanup_and_exit
			fi
		fi
	elif [ "$RETVAL" != "0" ]; then #return value expected to be 0
		echo $COMMAND_STRING Unexpected failure $COMMAND_DESCRIPTION >&2
		echo $FAIL
		cleanup_and_exit
	fi	
}

start () {
	if [ $# -ne 2 ]
	then
		echo "ERROR: Not enough arguments."
		cleanup_and_exit
	fi

	if [ ! -f "$1" ]
	then
		echo "ERROR: $1 is an invalid tbdiff-create path"  1>&2
		cleanup_and_exit
	fi

	if [ ! -f "$2" ]
	then
		echo "ERROR: $1 is an invalid tbdiff-deploy path" 1>&2
		cleanup_and_exit
	fi
}

cleanup_and_exit () {
	rm -rf $TESTDIR
	exit 1
}
command_succeeded () {
	test "$1" = "0"
}
main () {
	start "$@"
	echo -n "$TEST_ID Setting up $TEST_NAME test: "
	if [ ! -d $TESTDIR ]
	then
		echo $FAIL
		echo "Couldn't create temporary directory for test. " \
		     "Please check mktemp accepts -d and permissions." >&2
		cleanup_and_exit
	fi
	mkdir -p $ORIGIN &&
	check_command 'setup_origin' "$TEST_ID-$TEST_NAME: creating origin" \
	              'command_succeeded' &&
	mkdir -p $TARGET &&
	check_command 'setup_target' "$TEST_ID-$TEST_NAME: creating target" \
	              'command_succeeded' &&

	echo $OK

	echo "$TEST_ID Performing $TEST_NAME image creation and deployment: "
	sleep 2s &&
	CWD=$(pwd) &&
	check_command "$CREATE $IMGFILE $ORIGIN $TARGET" \
	              "$TEST_ID-$TEST_NAME: creating image" \
	              'create_test_return'

	cd $ORIGIN && 
	check_command "$DEPLOY $IMGFILE"  \
	              "$TEST_ID-$TEST_NAME: deploying image" \
	              'deploy_test_return'

	cd $CWD
	echo -n "$TEST_ID Checking $TEST_NAME results: "
	check_results
	if test "x$?" != "x0"
	then
		echo $FAIL
		echo "Applying image did not produce the expected results" 1>&2
		cleanup_and_exit
	fi
	echo $OK
}
