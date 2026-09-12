#!/bin/sh

set -eu

test_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
root_dir=$(CDPATH= cd -- "$test_dir/../../.." && pwd)
header="$root_dir/os/common/ports/ARMv8-M-ML-ALT/smp/rp2/chcoresmp.h"
cc=${CC:-cc}

preprocess_spinlocks() {
  kernel_lock=$1
  lockout_lock=$2

  "$cc" -E -x c -include "$header" \
    -DPORT_SPINLOCK_NUMBER="$kernel_lock" \
    -DPORT_LOCKOUT_SPINLOCK_NUMBER="$lockout_lock" \
    /dev/null >/dev/null
}

expect_pass() {
  label=$1
  shift

  if ! preprocess_spinlocks "$@"; then
    echo "unexpected rejected configuration: $label" >&2
    exit 1
  fi
}

expect_fail() {
  label=$1
  expected=$2
  output=
  shift 2

  if output=$(preprocess_spinlocks "$@" 2>&1); then
    echo "unexpected accepted configuration: $label" >&2
    exit 1
  fi
  case "$output" in
  *"$expected"*)
    ;;
  *)
    echo "unexpected rejection reason: $label" >&2
    printf '%s\n' "$output" >&2
    exit 1
    ;;
  esac
}

expect_pass "safe lower override"           5 18
expect_pass "safe singleton override"      10 30
expect_fail "unsafe kernel boundary" \
  "PORT_SPINLOCK_NUMBER is unsafe"         17 30
expect_fail "unsafe lockout gap" \
  "PORT_LOCKOUT_SPINLOCK_NUMBER is unsafe" 31 8
expect_fail "conflicting safe locks" \
  "PORT_LOCKOUT_SPINLOCK_NUMBER conflicts" 30 30

echo "RP2350 spinlock configuration checks passed"
