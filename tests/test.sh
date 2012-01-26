#!/bin/bash

if [ ! -f run_tests.sh ]
then
    echo "Test needs to be run from the tests directory" 1>&2
    exit 1
fi
if [ ! -f test_lib.sh ]
then
    echo "Could not find test_lib.sh" 1>&2
    exit 1
fi
if [ ! $(id -u) -eq 0 ] 
then
    echo "Test needs to be run as root" 1>&2
    exit 1
fi
if [ ! -d PATCHES ];
then
    echo "Could not find test patches. Check tests/PATCHES or run tests/gen.sh\
             to generate new patches for testing" 1>&2
    exit 1
fi

error() {
    echo "Failed"
    cd $_TESTFOLDER
#    cleanup_and_exit
    exit 1
}

_TESTFOLDER=`pwd`

. ./test_lib.sh

#trap error ERR

echo -e "\n#### Running Cross-platform Test"

echo -e "\nPreparing Test Environment... "
mkdir balls
cd $_TESTFOLDER/balls
for ((i=0; i<8; i++)); do
    mkdir ${nums[$i]}
done
for ((i=1; i<7; i++)); do
    cd ${nums[$i]} 
    echo -e "\nI'M IN ${nums[$i]}!!\n"
    for ((j=1; j<=i; j++)); do
        tbdiff-deploy $_TESTFOLDER/PATCHES/patch${nums[$j]}.tbd
    done
    cd ..
done

echo -e "\nCreating Patches... "
mkdir -p TESTPATCHES
for ((i=1; i<8; i++)); do
    tbdiff-create TESTPATCHES/primepatch${nums[$i]}.tbd ${nums[$i]} ${nums[$(($i+1))]}
done

for ((i=1; i<7; i++)); do
    echo -en "Checking Patch $ii..."
    diff TESTPATCHES/primepatch${nums[$i]}.tbd $_TESTFOLDER/PATCHES/patch${nums[$j]}.tbd
    if [ $? -eq 0 ]; then
        echo OK
    else
        error
    fi
done
