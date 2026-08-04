#!/bin/sh

set -eu

test_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
root_dir=$(CDPATH= cd -- "$test_dir/../../.." && pwd)
mock_dir="$test_dir/vt_tickless_mock"
build_dir=$(mktemp -d)
cc=${CC:-cc}

trap 'rm -rf "$build_dir"' EXIT HUP INT TERM

"$cc" -std=c99 -Wall -Wextra -Werror -pedantic \
  -I "$mock_dir" \
  -I "$root_dir/os/rt/include" \
  "$root_dir/os/rt/src/chvt.c" \
  "$mock_dir/main.c" \
  -o "$build_dir/vt_tickless_mock"

"$build_dir/vt_tickless_mock"
