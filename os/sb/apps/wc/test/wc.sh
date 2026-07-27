#!/bin/sh

set -eu

test_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
. "$test_dir/../../common/test/testlib.sh"

if [ "$#" -ne 1 ]; then
  sbtest_fail "usage: $0 <wc-executable>"
fi

case $1 in
/*) wc_exe=$1 ;;
*)  wc_exe=$(pwd)/$1 ;;
esac

case_dir=$(mktemp -d "${TMPDIR:-/tmp}/sb-wc-test.XXXXXX")
trap 'rm -rf "$case_dir"' EXIT HUP INT TERM

printf 'one two\nthree\n' >"$case_dir/input"

output=$("$wc_exe" "$case_dir/input")
sbtest_contains "$output" "2 3 14 $case_dir/input" "default counts differ"

output=$("$wc_exe" -l "$case_dir/input")
sbtest_contains "$output" "2 $case_dir/input" "line count differs"

output=$("$wc_exe" -cw "$case_dir/input")
sbtest_contains "$output" "3 14 $case_dir/input" "selected counts differ"

output=$("$wc_exe" <"$case_dir/input")
sbtest_contains "$output" "2 3 14" "standard-input counts differ"

if "$wc_exe" "$case_dir/missing" >/dev/null 2>&1; then
  sbtest_fail "missing file returned success"
fi

echo "wc behavioral checks passed"
