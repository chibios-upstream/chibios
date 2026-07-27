#!/bin/sh

set -eu

test_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
. "$test_dir/../../common/test/testlib.sh"

if [ "$#" -ne 1 ]; then
  sbtest_fail "usage: $0 <chedit-executable>"
fi

case $1 in
/*) chedit_exe=$1 ;;
*)  chedit_exe=$(pwd)/$1 ;;
esac

version=$("$chedit_exe" --version | tr -d '\r')
[ "$version" = "chedit 0.1.0" ] ||
  sbtest_fail "unexpected version output"

"$chedit_exe" --help | tr -d '\r' |
  grep -q '^usage: chedit \[file\]$' ||
  sbtest_fail "help output lacks usage"

if "$chedit_exe" one two >/dev/null 2>&1; then
  sbtest_fail "extra operand returned success"
fi

echo "chedit command-line checks passed"
