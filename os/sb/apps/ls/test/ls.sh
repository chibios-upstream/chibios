#!/bin/sh

set -eu

test_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
. "$test_dir/../../common/test/testlib.sh"

if [ "$#" -ne 1 ]; then
  sbtest_fail "usage: $0 <ls-executable>"
fi

case $1 in
/*)
  ls_exe=$1
  ;;
*)
  ls_exe=$(pwd)/$1
  ;;
esac

case_dir=$(mktemp -d "${TMPDIR:-/tmp}/sb-ls-test.XXXXXX")
trap 'rm -rf "$case_dir"' EXIT HUP INT TERM

touch "$case_dir/normal"
touch "$case_dir/.hidden"
touch "$case_dir/-dash"
touch "$case_dir/aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
mkdir "$case_dir/subdir"

output=$("$ls_exe" "$case_dir")
sbtest_not_contains "$output" "$(printf '\r')" "listing contains CR"
sbtest_contains "$output" "normal" "default listing omitted a regular file"
sbtest_contains "$output" "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa" \
  "default listing omitted a long file name"
sbtest_not_contains "$output" ".hidden" "default listing included a hidden file"

output=$("$ls_exe" -a "$case_dir")
sbtest_contains "$output" ".hidden" "-a listing omitted a hidden file"

output=$("$ls_exe" -l "$case_dir/normal")
sbtest_contains "$output" "$case_dir/normal" "-l listing omitted its operand"

output=$(cd "$case_dir" && "$ls_exe" -- -dash)
sbtest_equals "$output" "-dash" "-- did not terminate option parsing"

if output=$("$ls_exe" "$case_dir/missing" 2>&1); then
  sbtest_fail "missing operand returned success"
fi
sbtest_not_contains "$output" "$(printf '\r')" "diagnostic contains CR"

# Ordering, the times are UTC.
order_dir=$case_dir/order
mkdir "$order_dir"
head -c 300 /dev/zero >"$order_dir/a"
head -c 100 /dev/zero >"$order_dir/b"
head -c 200 /dev/zero >"$order_dir/c"
chmod 600 "$order_dir/a" "$order_dir/b" "$order_dir/c"
TZ=UTC touch -d '2020-01-02 03:04:05' "$order_dir/a"
TZ=UTC touch -d '2022-06-30 23:59:58' "$order_dir/b"
TZ=UTC touch -d '2021-12-31 00:00:00' "$order_dir/c"
nl=$(printf '\n.')
nl=${nl%.}

output=$("$ls_exe" "$order_dir")
sbtest_equals "$output" "a${nl}b${nl}c" "names not sorted"
output=$("$ls_exe" -r "$order_dir")
sbtest_equals "$output" "c${nl}b${nl}a" "-r did not reverse"
output=$("$ls_exe" -t "$order_dir")
sbtest_equals "$output" "b${nl}c${nl}a" "-t did not sort newest first"
output=$("$ls_exe" -tr "$order_dir")
sbtest_equals "$output" "a${nl}c${nl}b" "-tr did not sort oldest first"
output=$("$ls_exe" -S "$order_dir")
sbtest_equals "$output" "a${nl}c${nl}b" "-S did not sort largest first"

# Long format: mode, links, aligned size, UTC time and name, a zero time
# is shown as "-".
head -c 7 /dev/zero >"$order_dir/d"
chmod 600 "$order_dir/d"
touch -d '@0' "$order_dir/d"
output=$("$ls_exe" -l "$order_dir")
sbtest_contains "$output" "${nl}-rw------- 1 300 2020-01-02 03:04 a${nl}" \
  "-l line incorrect"
sbtest_contains "$output" "${nl}-rw------- 1 200 2021-12-31 00:00 c${nl}" \
  "-l size not aligned"
sbtest_contains "$output" "${nl}-rw------- 1   7 -                d" \
  "-l zero time not shown as -"
case $output in
"total "[0-9]*) ;;
*) sbtest_fail "-l directory listing without total" ;;
esac
output=$("$ls_exe" -l "$order_dir/a")
sbtest_equals "$output" "-rw------- 1 300 2020-01-02 03:04 $order_dir/a" \
  "-l file operand incorrect"

# Allocated size before the names.
output=$("$ls_exe" -s "$order_dir")
if ! printf '%s\n' "$output" | grep -Eq '^ *[0-9]+ a$'; then
  sbtest_fail "-s did not print the allocated size"
fi
case $output in
"total "[0-9]*) ;;
*) sbtest_fail "-s directory listing without total" ;;
esac

# Type suffixes.
mkfifo "$order_dir/fifo"
chmod 700 "$order_dir/b"
output=$("$ls_exe" -F "$case_dir" "$order_dir")
sbtest_contains "$output" "${nl}subdir/${nl}" "-F directory suffix missing"
sbtest_contains "$output" "${nl}b*${nl}" "-F executable suffix missing"
sbtest_contains "$output" "${nl}fifo|" "-F FIFO suffix missing"

# File operands first, then the directories with headers.
rm "$order_dir/d" "$order_dir/fifo"
output=$("$ls_exe" "$order_dir" "$order_dir/c")
sbtest_equals "$output" \
  "$order_dir/c${nl}${nl}$order_dir:${nl}a${nl}b${nl}c" \
  "operand order or headers incorrect"
output=$("$ls_exe" -d "$order_dir")
sbtest_equals "$output" "$order_dir" "-d listed the directory contents"

# Failed operands are reported, the others listed, exit status 1.
status=0
output=$("$ls_exe" "$order_dir/missing" "$order_dir/a" 2>/dev/null) || status=$?
sbtest_equals "$status" "1" "failed operand exit status"
sbtest_equals "$output" "$order_dir/a" "remaining operand not listed"

status=0
"$ls_exe" -x "$order_dir" >/dev/null 2>&1 || status=$?
sbtest_equals "$status" "2" "invalid option exit status"

echo "ls behavioral checks passed"
