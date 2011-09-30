#!/bin/bash

if [ ! -f $TESTLIB ]
then
	echo "Could not find test_lib.sh" 1>&2
fi

for i in [0-9][0-9]*
do
	./$i $1 $2
	if [ $? -ne 0 ]
	then
		exit 1
	fi
done
