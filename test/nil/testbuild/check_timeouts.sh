#!/bin/sh

set -eu

test_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
root_dir=$(CDPATH= cd -- "$test_dir/../../.." && pwd)
mock_dir="$test_dir/timeouts_mock"
build_dir=$(mktemp -d)
cc=${CC:-cc}

trap 'rm -f "$build_dir/timeouts"; rmdir "$build_dir"' EXIT HUP INT TERM

for assertions in TRUE FALSE; do
  for bits in 16 32; do
    for delta in 0 2 10; do
      "$cc" -std=c99 -O2 -Wall -Wextra -Werror \
        -ffunction-sections -fdata-sections \
        -DCH_DBG_ENABLE_ASSERTS="$assertions" \
        -DCH_CFG_ST_RESOLUTION="$bits" -DCH_CFG_ST_TIMEDELTA="$delta" \
        -I "$mock_dir/cfg" -I "$mock_dir" \
        -I "$root_dir/os/nil/include" \
        -I "$root_dir/os/common/ports/SIMX86_64/compilers/GCC" \
        -I "$root_dir/os/common/portability/GCC" \
        -I "$root_dir/os/license" -I "$root_dir/os/oslib/include" \
        "$root_dir/os/nil/src/ch.c" "$root_dir/os/nil/src/chsem.c" \
        "$root_dir/os/nil/src/chevt.c" "$root_dir/os/nil/src/chmsg.c" \
        "$mock_dir/main.c" -Wl,--gc-sections -o "$build_dir/timeouts"
      "$build_dir/timeouts" "$@"
    done
  done
done
