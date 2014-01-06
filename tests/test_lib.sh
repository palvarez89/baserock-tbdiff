OK=" OK"
FAIL=" FAIL"

TESTDIR="$PWD/temp"
rm -rf "$TESTDIR"
mkdir "$TESTDIR"

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

cleanup_and_exit () {
       rm -rf $TESTDIR
       exit 1
}
