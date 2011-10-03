#!/bin/bash

if [ ! -f run_tests.sh ]
then
	echo "Test suite needs to be run from the tests directory" 1>&2
	exit 1
fi

if [ ! -f test_lib.sh ]
then
	echo "Could not find test_lib.sh" 1>&2
	exit 1
fi

ALLTESTSDIR=`pwd`

for i in [0-9][0-9]*
do
	cd $ALLTESTSDIR
	echo "#### Running $i"
	fakeroot -- ./$i ../tbdiff-create ../tbdiff-deploy
	if [ $? -ne 0 ]
	then
		echo "Test program $i failed" 1>&2
		cd $ALLTESTSDIR
		echo "-^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^-"
		exit 1
	fi
		echo "#####################################################################"
done
