#!/bin/sh

set -eu

test_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
. "$test_dir/../../common/test/testlib.sh"

if [ "$#" -ne 1 ]; then
  sbtest_fail "usage: $0 <hexdump-executable>"
fi

case $1 in
/*) hexdump_exe=$1 ;;
*)  hexdump_exe=$(pwd)/$1 ;;
esac

case_dir=$(mktemp -d "${TMPDIR:-/tmp}/sb-hexdump-test.XXXXXX")
trap 'rm -rf "$case_dir"' EXIT HUP INT TERM

printf 'A\000z' >"$case_dir/input"

output=$("$hexdump_exe" "$case_dir/input")
sbtest_contains "$output" "00000000" "initial offset is missing"
sbtest_contains "$output" "41 00 7a" "hexadecimal bytes differ"
sbtest_contains "$output" "|A.z" "printable representation differs"

output=$("$hexdump_exe" <"$case_dir/input")
sbtest_contains "$output" "41 00 7a" "standard-input dump differs"

if "$hexdump_exe" "$case_dir/missing" >/dev/null 2>&1; then
  sbtest_fail "missing file returned success"
fi
if "$hexdump_exe" "$case_dir/input" "$case_dir/input" >/dev/null 2>&1; then
  sbtest_fail "extra operand returned success"
fi

echo "hexdump behavioral checks passed"
