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

chmod 640 "$case_dir/file"
TZ=UTC touch -d '2024-02-29 12:34:56' "$case_dir/file"

output=$("$stat_exe" "$case_dir/file")
sbtest_contains "$output" "  File: $case_dir/file" "path is missing"
sbtest_contains "$output" "  Type: file" "regular-file type is missing"
sbtest_contains "$output" "  Mode: 0640 (-rw-r-----)  Links: 1" \
  "mode or links incorrect"
sbtest_contains "$output" "  Size: 3  Blocks: " "size is missing"
sbtest_contains "$output" "  IO Block: " "I/O block size is missing"
sbtest_contains "$output" "Modify: 2024-02-29 12:34:56 UTC" \
  "modification time incorrect"

touch -d '@0' "$case_dir/file"
output=$("$stat_exe" "$case_dir/file")
sbtest_contains "$output" "Modify: -" "zero time not shown as -"

output=$("$stat_exe" "$case_dir/directory")
sbtest_contains "$output" "  Type: directory" "directory type is missing"

if "$stat_exe" "$case_dir/missing" >/dev/null 2>&1; then
  sbtest_fail "missing path returned success"
fi
if "$stat_exe" >/dev/null 2>&1; then
  sbtest_fail "missing operand returned success"
fi

echo "stat behavioral checks passed"
