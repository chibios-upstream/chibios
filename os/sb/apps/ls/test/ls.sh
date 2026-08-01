#!/bin/sh

set -eu

test_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
. "$test_dir/../../common/test/testlib.sh"

if [ "$#" -ne 1 ]; then
  sbtest_fail "usage: $0 <ls-executable>"
fi

case $1 in
/*)
  ls_exe=$1
  ;;
*)
  ls_exe=$(pwd)/$1
  ;;
esac

case_dir=$(mktemp -d "${TMPDIR:-/tmp}/sb-ls-test.XXXXXX")
trap 'rm -rf "$case_dir"' EXIT HUP INT TERM

touch "$case_dir/normal"
touch "$case_dir/.hidden"
touch "$case_dir/-dash"
touch "$case_dir/aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
mkdir "$case_dir/subdir"

output=$("$ls_exe" "$case_dir")
sbtest_contains "$output" "normal" "default listing omitted a regular file"
sbtest_contains "$output" "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa" \
  "default listing omitted a long file name"
sbtest_not_contains "$output" ".hidden" "default listing included a hidden file"

output=$("$ls_exe" -a "$case_dir")
sbtest_contains "$output" ".hidden" "-a listing omitted a hidden file"

output=$("$ls_exe" -l "$case_dir/normal")
sbtest_contains "$output" "$case_dir/normal" "-l listing omitted its operand"

output=$(cd "$case_dir" && "$ls_exe" -- -dash)
sbtest_contains "$output" "-dash" "-- did not terminate option parsing"

if "$ls_exe" "$case_dir/missing" >/dev/null 2>&1; then
  sbtest_fail "missing operand returned success"
fi

echo "ls behavioral checks passed"
