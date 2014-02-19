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

compare_dirs() {

    # Temporary files used to compare the file permissions
    file1=$(mktemp)
    file2=$(mktemp)

    set +e
    (
        set -e
        # Getting the file permissions
        (cd "$1" && busybox find * -exec busybox stat -c '%n %a' {} + | sort) > "$file1"
        (cd "$2" && busybox find * -exec busybox stat -c '%n %a' {} + | sort) > "$file2"

        # Compare file contents
        diff -r "$1" "$2"

        # Compare permissions
        diff "$file1" "$file2"
    )
    local ret="$?"

    # Clean temporary files
    rm "$file1"
    rm "$file2"

    return $ret
}

echo "Starting baserock-system-config-sync tests"
merge_pass_folder="bscs-merge.pass"
merge_fail_folder="bscs-merge.fail"
sync_folder="bscs-sync"
bscs_script="../baserock-system-config-sync/baserock-system-config-sync"
bscs_log="bscs.log"
> "$bscs_log"

# test merge cases that should succeed
for folder in "$merge_pass_folder/"*.in; do
    echo -n "#### Running ${folder%.in}"
    echo "#### Running ${folder%.in}" >> "$bscs_log"
    out_folder=${folder%.in}.out
    TMPDIR=$(mktemp -d)
    TMPDIR=$TMPDIR mounting_script="./fake_mounting_script.sh" unmount=true \
        mounting_script_test_dir="$folder" "$bscs_script" "merge" \
        "version2" &>> "$bscs_log"
    exit_code="$?"
    if [ "$exit_code" -ne 0 ]; then
        echo ": FAILED (exit code "$exit_code")" 1>&2
        exit 1
    elif ! compare_dirs "$TMPDIR/"*/ "$out_folder/" &>> "$bscs_log"; then
        echo ": FAILED (different diff)" 1>&2
        exit 1
    else
        echo ": OK" 1>&2
    fi
    rm -rf $TMPDIR
done


# test merge changes that should fail
for folder in "$merge_fail_folder/"*.in; do
    echo -n "#### Running ${folder%.in}"
    echo "#### Running ${folder%.in}" >> "$bscs_log"
    TMPDIR=$(mktemp -d)
    TMPDIR=$TMPDIR mounting_script="./fake_mounting_script.sh" unmount=true \
        mounting_script_test_dir="$folder" "$bscs_script" "merge" \
        "version2" &>> "$bscs_log"
    if [ $? -eq 0 ]; then
        echo ": FAILED" 1>&2
        exit 1
    else
        echo ": OK" 1>&2
    fi
    rm -rf $TMPDIR
done


# test the sync mode
echo -n "#### Running sync test on $sync_folder"
echo "#### Running sync test on $sync_folder" >> "$bscs_log"
out_folder=${sync_folder}.out
TMPDIR=$(mktemp -d)
TMPDIR=$TMPDIR mounting_script="./fake_mounting_script.sh" unmount=true \
    mounting_script_test_dir="${sync_folder}.in" "$bscs_script" "sync" \
    "version3" &>> "$bscs_log"
exit_code="$?"
if [ "$exit_code" -ne 0 ]; then
    echo ": FAILED (exit code "$exit_code")" 1>&2
    exit 1
elif ! compare_dirs "$TMPDIR/"*/ "$out_folder/" &>> "$bscs_log"; then
    echo ": FAILED (different diff)" 1>&2a
    exit 1
else
   echo ": OK" 1>&2
fi
rm -rf $TMPDIR
