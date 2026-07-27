#!/bin/sh

sbtest_fail() {
  echo "error: $*" >&2
  exit 1
}

sbtest_contains() {
  case $1 in
  *"$2"*)
    ;;
  *)
    sbtest_fail "$3"
    ;;
  esac
}

sbtest_not_contains() {
  case $1 in
  *"$2"*)
    sbtest_fail "$3"
    ;;
  *)
    ;;
  esac
}

sbtest_equals() {
  if [ "$1" != "$2" ]; then
    sbtest_fail "$3: expected '$2', got '$1'"
  fi
}
