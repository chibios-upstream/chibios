#!/bin/sh

set -eu

test_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
root_dir=$(CDPATH= cd -- "$test_dir/../../.." && pwd)
mock_dir="$test_dir/vt_tickless_mock"
build_dir=$(mktemp -d)
cc=${CC:-cc}

trap 'rm -rf "$build_dir"' EXIT HUP INT TERM

# Exercise real reciprocal-link and pointer checks in the mock builds too.
for hardening in 0 2 3; do
  "$cc" -std=c99 -Wall -Wextra -Werror -pedantic \
    -DCH_CFG_HARDENING_LEVEL="$hardening" \
    -I "$mock_dir" \
    -I "$root_dir/os/rt/include" \
    "$root_dir/os/rt/src/chvt.c" \
    "$mock_dir/main.c" \
    -o "$build_dir/vt_tickless_mock"

  "$build_dir/vt_tickless_mock"

  for rfcu in 0 1; do
    for assertions in 0 1; do
      "$cc" -std=c99 -Wall -Wextra -Werror -pedantic \
        -DCH_CFG_HARDENING_LEVEL="$hardening" \
        -DCH_CFG_USE_RFCU="$rfcu" \
        -DCH_DBG_ENABLE_ASSERTS="$assertions" \
        -I "$mock_dir" \
        -I "$root_dir/os/rt/include" \
        "$root_dir/os/rt/src/chvt.c" \
        "$mock_dir/rfcu.c" \
        -o "$build_dir/vt_rfcu_mock"

      "$build_dir/vt_rfcu_mock"
    done
  done
done
