#!/bin/sh

set -eu

test_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
. "$test_dir/../../common/test/testlib.sh"

if [ "$#" -ne 1 ]; then
  sbtest_fail "usage: $0 <sbsh-executable>"
fi

case $1 in
/*) sbsh_exe=$1 ;;
*)  sbsh_exe=$(pwd)/$1 ;;
esac

work_dir=$(mktemp -d)
trap 'rm -rf -- "$work_dir"' EXIT HUP INT TERM

output=$("$sbsh_exe" -c 'echo one; echo "two words"; echo three\ four' |
         tr -d '\r')
expected=$(printf 'one\ntwo words\nthree four')
sbtest_equals "$output" "$expected" "sequential command output"

output=$("$sbsh_exe" -c 'echo a#b; echo before # ignored; echo after' |
         tr -d '\r')
expected=$(printf 'a#b\nbefore')
sbtest_equals "$output" "$expected" "comment parsing"

: >"$work_dir/alpha.txt"
: >"$work_dir/-option.txt"
: >"$work_dir/*a.mix"
: >"$work_dir/za.mix"
output=$("$sbsh_exe" -c "cd $work_dir; echo alpha*.txt; echo \"*.txt\"; echo *.none; echo \\*.txt; echo -*.txt" |
         tr -d '\r')
expected=$(printf 'alpha.txt\n*.txt\n*.none\n*.txt\n-option.txt')
sbtest_equals "$output" "$expected" "quote-aware glob expansion"

output=$("$sbsh_exe" -c "cd $work_dir; echo \"*\"?.mix" | tr -d '\r')
sbtest_equals "$output" "*a.mix" "mixed quoted glob expansion"

output=$("$sbsh_exe" -c "cd $work_dir; echo first > output; echo second >> output; /bin/cat < output" |
         tr -d '\r')
expected=$(printf 'first\nsecond')
sbtest_equals "$output" "$expected" "file redirection"

output=$(TMPDIR="$work_dir" "$sbsh_exe" -c \
         'echo first; echo second | /usr/bin/tr a-z A-Z | /usr/bin/head -n 1' |
         tr -d '\r')
expected=$(printf 'first\nSECOND')
sbtest_equals "$output" "$expected" "serialized pipeline"
for path in "$work_dir"/.sbsh-pipe-*; do
  [ ! -e "$path" ] ||
    sbtest_fail "pipeline temporary file was not removed"
done

status=0
TMPDIR="$work_dir" "$sbsh_exe" -c \
  'echo temporary | /no/such/sbsh-command' >/dev/null 2>&1 ||
  status=$?
sbtest_equals "$status" "127" "failed pipeline status"
for path in "$work_dir"/.sbsh-pipe-*; do
  [ ! -e "$path" ] ||
    sbtest_fail "failed pipeline left a temporary file"
done

printf 'echo script-first\r\n/bin/sh -c "exit 2"\r\necho script-last\r\n' \
  >"$work_dir/continue.sbsh"
status=0
raw_output=$("$sbsh_exe" "$work_dir/continue.sbsh") ||
  status=$?
output=$(printf '%s' "$raw_output" | tr -d '\r')
sbtest_equals "$status" "0" "script final status"
expected=$(printf 'script-first\nscript-last')
sbtest_equals "$output" "$expected" "script sequential execution"

printf 'echo before\n|\necho after\n' >"$work_dir/error.sbsh"
status=0
raw_output=$("$sbsh_exe" "$work_dir/error.sbsh" 2>&1) ||
  status=$?
output=$(printf '%s' "$raw_output" | tr -d '\r')
sbtest_equals "$status" "2" "script syntax status"
sbtest_contains "$output" "$work_dir/error.sbsh:2: syntax error" \
  "script syntax diagnostic lacks source location"
sbtest_not_contains "$output" "after" \
  "script continued after a syntax error"

status=0
raw_output=$("$sbsh_exe" -c 'cd / | echo invalid' 2>&1) ||
  status=$?
output=$(printf '%s' "$raw_output" | tr -d '\r')
sbtest_equals "$status" "1" "stateful pipeline status"
sbtest_contains "$output" "builtin cannot be used in a pipeline: cd" \
  "stateful pipeline was not diagnosed"

status=0
"$sbsh_exe" -c '/bin/sh -c "exit 2"' >/dev/null 2>&1 ||
  status=$?
sbtest_equals "$status" "2" "external command status"

status=0
"$sbsh_exe" -c 'exit 7' >/dev/null 2>&1 ||
  status=$?
sbtest_equals "$status" "7" "exit builtin status"

status=0
"$sbsh_exe" -c 'exit 256' >/dev/null 2>&1 ||
  status=$?
sbtest_equals "$status" "2" "invalid exit status"

echo "sbsh checks passed"
