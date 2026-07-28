#!/bin/sh

set -eu

test_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
. "$test_dir/../../common/test/testlib.sh"

if [ "$#" -ne 1 ]; then
  sbtest_fail "usage: $0 <head-executable>"
fi

case $1 in
/*) head_exe=$1 ;;
*)  head_exe=$(pwd)/$1 ;;
esac

case_dir=$(mktemp -d "${TMPDIR:-/tmp}/sb-head-test.XXXXXX")
trap 'rm -rf "$case_dir"' EXIT HUP INT TERM

printf 'one\ntwo\nthree\nfour\n' >"$case_dir/input"
printf 'one\ntwo\n' >"$case_dir/expected"

"$head_exe" -n 2 "$case_dir/input" >"$case_dir/output"
cmp -s "$case_dir/expected" "$case_dir/output" ||
  sbtest_fail "-n 2 output differs"

printf 'one\n' >"$case_dir/expected"
"$head_exe" -n 1 <"$case_dir/input" >"$case_dir/output"
cmp -s "$case_dir/expected" "$case_dir/output" ||
  sbtest_fail "standard-input output differs"

"$head_exe" -n 0 "$case_dir/input" >"$case_dir/output"
if [ -s "$case_dir/output" ]; then
  sbtest_fail "-n 0 produced output"
fi

if "$head_exe" -n bad "$case_dir/input" >/dev/null 2>&1; then
  sbtest_fail "invalid line count returned success"
fi

echo "head behavioral checks passed"
