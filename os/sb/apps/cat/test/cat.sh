#!/bin/sh

set -eu

test_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
. "$test_dir/../../common/test/testlib.sh"

if [ "$#" -ne 1 ]; then
  sbtest_fail "usage: $0 <cat-executable>"
fi

case $1 in
/*) cat_exe=$1 ;;
*)  cat_exe=$(pwd)/$1 ;;
esac

case_dir=$(mktemp -d "${TMPDIR:-/tmp}/sb-cat-test.XXXXXX")
trap 'rm -rf "$case_dir"' EXIT HUP INT TERM

printf 'alpha\nbeta\n' >"$case_dir/one"
printf 'gamma\n' >"$case_dir/two"
printf 'alpha\nbeta\ngamma\n' >"$case_dir/expected"

"$cat_exe" "$case_dir/one" "$case_dir/two" >"$case_dir/output"
cmp -s "$case_dir/expected" "$case_dir/output" ||
  sbtest_fail "multiple-file output differs"

printf 'stdin data\n' >"$case_dir/stdin"
"$cat_exe" <"$case_dir/stdin" >"$case_dir/output"
cmp -s "$case_dir/stdin" "$case_dir/output" ||
  sbtest_fail "standard-input output differs"

printf 'dash\n' >"$case_dir/-dash"
(cd "$case_dir" && "$cat_exe" -- -dash >output)
cmp -s "$case_dir/-dash" "$case_dir/output" ||
  sbtest_fail "-- did not terminate option parsing"

if "$cat_exe" "$case_dir/missing" >/dev/null 2>&1; then
  sbtest_fail "missing file returned success"
fi

echo "cat behavioral checks passed"
