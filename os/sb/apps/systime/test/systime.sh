#!/bin/sh

set -eu

test_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
. "$test_dir/../../common/test/testlib.sh"

if [ "$#" -ne 1 ]; then
  sbtest_fail "usage: $0 <systime-executable>"
fi

case $1 in
/*) systime_exe=$1 ;;
*)  systime_exe=$(pwd)/$1 ;;
esac

output=$("$systime_exe")
sbtest_contains "$output" "ticks=" "tick count is missing"
sbtest_contains "$output" " frequency=" "tick frequency is missing"

frequency=${output##*frequency=}
frequency=$(printf '%s' "$frequency" | tr -d '\r')
if [ "$frequency" -le 0 ]; then
  sbtest_fail "tick frequency is not positive"
fi

if "$systime_exe" extra >/dev/null 2>&1; then
  sbtest_fail "extra operand returned success"
fi

echo "systime behavioral checks passed"
