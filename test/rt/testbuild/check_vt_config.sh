#!/bin/sh

set -eu

test_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
root_dir=$(CDPATH= cd -- "$test_dir/../../.." && pwd)
cc=${CC:-cc}

preprocess_vt_config() {
  resolution=$1
  intervals=$2
  delta=$3

  "$cc" -E -x c -include chvt.h \
    -I "$root_dir/os/rt/include" \
    -DTRUE=1 \
    -DFALSE=0 \
    -DCH_CFG_ST_RESOLUTION="$resolution" \
    -DCH_CFG_INTERVALS_SIZE="$intervals" \
    -DCH_CFG_ST_TIMEDELTA="$delta" \
    -DCH_CFG_TIME_QUANTUM=0 \
    -DCH_DBG_THREADS_PROFILING=0 \
    /dev/null >/dev/null
}

expect_pass() {
  label=$1
  shift

  if ! preprocess_vt_config "$@"; then
    echo "unexpected rejected configuration: $label" >&2
    exit 1
  fi
}

expect_fail() {
  label=$1
  shift

  if preprocess_vt_config "$@" 2>/dev/null; then
    echo "unexpected accepted configuration: $label" >&2
    exit 1
  fi
}

expect_pass "periodic mode"                  16 16 0
expect_pass "minimum tickless delta"         16 16 2
expect_pass "16-bit maximum"                 16 16 0xFF00U
expect_pass "16/32-bit maximum"              16 32 0xFF00U
expect_pass "32-bit maximum"                 32 32 0xFFFF0000U
expect_pass "32/64-bit maximum"              32 64 0xFFFF0000ULL
expect_pass "64-bit maximum"                 64 64 0xFFFFFFFF00000000ULL

expect_fail "negative delta"                 32 32 -1
expect_fail "reserved delta one"             32 32 1
expect_fail "above 16-bit maximum"            16 16 0xFF01U
expect_fail "16-bit narrowing to zero"        16 16 0x10000U
expect_fail "16-bit narrowing to one"         16 16 0x10001U
expect_fail "above 16/32-bit physical maximum" 16 32 0xFF01U
expect_fail "above 32-bit maximum"            32 32 0xFFFF0001U
expect_fail "32-bit narrowing to zero"        32 32 0x100000000ULL
expect_fail "32-bit narrowing to one"         32 32 0x100000001ULL
expect_fail "above 32/64-bit physical maximum" 32 64 0xFFFF0001ULL
expect_fail "above 64-bit maximum"            64 64 0xFFFFFFFF00000001ULL

echo "VT configuration boundary checks passed"
