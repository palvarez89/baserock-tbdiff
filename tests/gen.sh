#!/bin/bash
# This script generates seven folders with incremental
# file type additions. This setup allows for the creation
# of patches for cross-platform testing of tbdiff. These patches
# are then commited and recreated and compared on different
# platforms to check compatibility between different systems
# including endianness and architecture.

. ./test_lib.sh

labelmeta() {
    touch -ht 197001010100.42 $1
    chmod 607 $1
}

cleanup() {
    for ((i=0; i<8; i++)); do
        rm -rf ${nums[$i]}
    done
}

if [ ! -f run_tests.sh ]
then
    echo "Test needs to be run from the tests directory" 1>&2
    exit 1
fi

if [ ! $(id -u) -eq 0 ]
then
    echo "Root."
    exit 1
fi

if [ -d PATCHES ]
then
    echo -n "Overwrite Existing Patches? "
    read input
    input=`echo $input | tr '[:upper:]' '[:lower:]'`
    case "$input" in
        [Yy]|'') ;;
        [Nn]*) echo "Aborted."; exit 1;;
        *) echo "??";;
    esac
fi

rm -rf PATCHES

mkdir -p zero
mkdir -p PATCHES
for ((i=1; i<8; i++)); do
    mkdir -p ${nums[$i]}
    cd ${nums[$i]}
    insertfiles $i
#    for ((j=1; j<=$i; j++)); do
#        labelmeta ${files[$j]}
#    done
    cd ..
    tbdiff-create PATCHES/patch${nums[$i]}.tbd ${nums[$(($i - 1))]} ${nums[$i]}
done

cleanup

exit 0
