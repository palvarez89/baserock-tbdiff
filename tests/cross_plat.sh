#!/bin/bash
# Deploys the patch files in PATCHES, which were ideally generated on a x64
# Little-endian system using gen.sh. It then creates its own patches, the
# idea being that upon diffing the committed patches and these patches, they
# should be identical.
#
# It creates seven patches at the moment, one for each change made. This is
# because tbdiff doesn't sort files before it puts them into patches, so
# the order in which files are created can differ from system to system,
# making the patch files different even though they're the same. By limiting
# each patch to only one change, the patches will be identical.

# CURRENTLY BROKEN!!
# Patch seven seems to be failing the diff check for some reason.


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
    cleanup_and_exit
    exit 1
}

_TESTFOLDER=`pwd`

. ./test_lib.sh

#trap error ERR

echo -e "#### Running Cross-platform Test"

echo -e "\nPreparing Test Environment... "
#mkdir foo
#cd $_TESTFOLDER/foo
cd $TESTDIR
# Create 8 folders, one, two ...
for ((i=0; i<8; i++)); do
    mkdir ${nums[$i]}
done

# Deploy the patches in PATCHES/ to those folders
for ((i=1; i<7; i++)); do
    cd ${nums[$i]} 
#    echo -e "\nI'M IN ${nums[$i]}!!\n"
    for ((j=1; j<=i; j++)); do
        tbdiff-deploy $_TESTFOLDER/PATCHES/patch${nums[$j]}.tbd
    done
    cd ..
done

# Create your own patches
echo -e "\nCreating Patches... "
mkdir -p TESTPATCHES
for ((i=0; i<7; i++)); do
    tbdiff-create TESTPATCHES/primepatch${nums[$(($i+1))]}.tbd ${nums[$i]} ${nums[$(($i+1))]}
done

# Diff patches. Should be identical
for ((i=1; i<8; i++)); do
    echo -en "Checking patch ${nums[$i]}... "
    diff TESTPATCHES/primepatch${nums[$i]}.tbd $_TESTFOLDER/PATCHES/patch${nums[$i]}.tbd 2>/dev/null #1>&2
    if [ $? -eq 0 ]; then
        echo OK
    else
        error
    fi
done
