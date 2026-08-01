#!/bin/sh

set -eu

test_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
. "$test_dir/../../common/test/testlib.sh"

if [ "$#" -ne 1 ]; then
  sbtest_fail "usage: $0 <cmp-executable>"
fi

case $1 in
/*) cmp_exe=$1 ;;
*)  cmp_exe=$(pwd)/$1 ;;
esac

case_dir=$(mktemp -d "${TMPDIR:-/tmp}/sb-cmp-test.XXXXXX")
trap 'rm -rf "$case_dir"' EXIT HUP INT TERM

printf 'same\n' >"$case_dir/one"
printf 'same\n' >"$case_dir/two"
printf 'sXme\n' >"$case_dir/different"

"$cmp_exe" "$case_dir/one" "$case_dir/two" >"$case_dir/output"
if [ -s "$case_dir/output" ]; then
  sbtest_fail "equal files produced output"
fi

if "$cmp_exe" "$case_dir/one" "$case_dir/different" >"$case_dir/output"; then
  sbtest_fail "different files returned success"
else
  status=$?
fi
sbtest_equals "$status" "1" "different-file status"
output=$(cat "$case_dir/output")
sbtest_contains "$output" "byte 2" "difference position is wrong"

if "$cmp_exe" "$case_dir/one" "$case_dir/missing" >/dev/null 2>&1; then
  sbtest_fail "missing file returned success"
else
  status=$?
fi
sbtest_equals "$status" "2" "error status"

echo "cmp behavioral checks passed"
