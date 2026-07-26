#!/bin/sh

set -eu

test_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
. "$test_dir/../../common/test/testlib.sh"

if [ "$#" -ne 1 ]; then
  sbtest_fail "usage: $0 <stat-executable>"
fi

case $1 in
/*) stat_exe=$1 ;;
*)  stat_exe=$(pwd)/$1 ;;
esac

case_dir=$(mktemp -d "${TMPDIR:-/tmp}/sb-stat-test.XXXXXX")
trap 'rm -rf "$case_dir"' EXIT HUP INT TERM

printf 'abc' >"$case_dir/file"
mkdir "$case_dir/directory"

output=$("$stat_exe" "$case_dir/file")
sbtest_contains "$output" "file " "regular-file type is missing"
sbtest_contains "$output" " 3 $case_dir/file" "regular-file size or path is missing"

output=$("$stat_exe" "$case_dir/directory")
sbtest_contains "$output" "directory " "directory type is missing"

if "$stat_exe" "$case_dir/missing" >/dev/null 2>&1; then
  sbtest_fail "missing path returned success"
fi
if "$stat_exe" >/dev/null 2>&1; then
  sbtest_fail "missing operand returned success"
fi

echo "stat behavioral checks passed"
