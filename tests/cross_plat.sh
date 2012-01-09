#!/bin/bash

CWD=`pwd`

CREATE="../tbdiff-create"
DEPLOY="../tbdiff-deploy"

PATCH1="../patch1.tbd"
PATCH2="../patch2.tbd"

EMPTY="empty"
MIDDIR="middir"
FINALDIR="finaldir"

cleanup() {
    echo "Cleaning up..."
    cd $CWD
    rm -rf $EMPTY $MIDDIR $FINALDIR patch1prime.tbd patch2prime.tbd
}
checkmeta() {
    cd $CWD
    echo "Checking metadata..."
    cd $MIDDIR
    #1325779232 is the hardcoded creation time of b.txt and blockdev in patch1
    if [ `stat -c %Y b.txt` -eq 1325779232 ]; then
        echo "b.txt OK"
    else
        echo "Metadata mismatch"
        cleanup
        exit 1
    fi
    if [ `stat -c %Y dirdir/blockdev` -eq 1325779232 ]; then
        echo "blockdev OK"
    else
        echo "Metadata mismatch"
        cleanup
        exit 1
    fi
    cd ..
    cd $FINALDIR
    #1325769455 is the hardcoded creation time of a.txt and fifo in patch2
    if [ `stat -c %Y a.txt` -eq 1325769455 ]; then
        echo "a.txt OK"
    else
        echo "Metadata mismatch"
        cleanup
        exit 1
    fi
    if [ `stat -c %Y dirdir/fifo` -eq 1325769455 ]; then
        echo "fifo OK"
    else
        echo "Metadata mismatch"
        cleanup
        exit 1
    fi
    cd ..
}

echo -e "\n#### Preparing Cross Platform Test"
if [ ! -f run_tests.sh ]
then
    echo "Test needs to be run from the tests directory" 1>&2
    cleanup
    exit 1
fi

mkdir $EMPTY
mkdir $MIDDIR
cd $MIDDIR
../$DEPLOY $PATCH1
cd ..

cp -R $MIDDIR $FINALDIR
cd $FINALDIR
../$DEPLOY $PATCH2
cd ..

$CREATE patch1prime.tbd $EMPTY $MIDDIR
$CREATE patch2prime.tbd $MIDDIR $FINALDIR

echo -e "\n#### Testing ####"
echo "Checking Diffs..."
diff patch1.tbd patch1prime.tbd
if [ $? -eq 0 ]; then
    echo -n "Patch 1: "
    echo OK
fi
diff patch2.tbd patch2prime.tbd
if [ $? -eq 0 ]; then
    echo -n "Patch 2: "
    echo OK
fi

checkmeta
cleanup
echo "#####################################################################"

exit 0
