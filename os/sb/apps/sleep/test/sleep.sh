#!/bin/sh

set -eu

test_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
. "$test_dir/../../common/test/testlib.sh"

if [ "$#" -ne 1 ]; then
  sbtest_fail "usage: $0 <sleep-executable>"
fi

case $1 in
/*) sleep_exe=$1 ;;
*)  sleep_exe=$(pwd)/$1 ;;
esac

"$sleep_exe" 0

if "$sleep_exe" bad >/dev/null 2>&1; then
  sbtest_fail "invalid duration returned success"
fi
if "$sleep_exe" -1 >/dev/null 2>&1; then
  sbtest_fail "negative duration returned success"
fi
if "$sleep_exe" 0 extra >/dev/null 2>&1; then
  sbtest_fail "extra operand returned success"
fi

echo "sleep behavioral checks passed"
