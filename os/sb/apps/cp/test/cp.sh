#!/bin/sh

set -eu

test_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
. "$test_dir/../../common/test/testlib.sh"

if [ "$#" -ne 1 ]; then
  sbtest_fail "usage: $0 <cp-executable>"
fi

case $1 in
/*) cp_exe=$1 ;;
*)  cp_exe=$(pwd)/$1 ;;
esac

case_dir=$(mktemp -d "${TMPDIR:-/tmp}/sb-cp-test.XXXXXX")
trap 'rm -rf "$case_dir"' EXIT HUP INT TERM

printf 'source data\n' >"$case_dir/source"
"$cp_exe" "$case_dir/source" "$case_dir/destination"
cmp -s "$case_dir/source" "$case_dir/destination" ||
  sbtest_fail "new destination differs"

printf 'a much longer old destination\n' >"$case_dir/destination"
"$cp_exe" "$case_dir/source" "$case_dir/destination"
cmp -s "$case_dir/source" "$case_dir/destination" ||
  sbtest_fail "existing destination was not replaced"

if "$cp_exe" "$case_dir/source" "$case_dir/source" >/dev/null 2>&1; then
  sbtest_fail "identical source and destination returned success"
fi
if "$cp_exe" "$case_dir/missing" "$case_dir/other" >/dev/null 2>&1; then
  sbtest_fail "missing source returned success"
fi

echo "cp behavioral checks passed"
