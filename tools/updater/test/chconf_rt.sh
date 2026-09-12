#!/bin/sh
# Run with: sh tools/updater/test/chconf_rt.sh
set -eu

test_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
updater_dir=$(CDPATH= cd -- "$test_dir/.." && pwd)
repo_dir=$(CDPATH= cd -- "$updater_dir/../.." && pwd)
case_dir=$(mktemp -d "${TMPDIR:-/tmp}/chconf-rt-test.XXXXXX")
trap 'rm -rf -- "$case_dir"' EXIT HUP INT TERM

# The updater uses relative paths and temporary files; isolate its workspace.
mkdir "$case_dir/updater"
ln -s "$repo_dir/tools/ftl" "$case_dir/ftl"
cp "$updater_dir/conf.fmpp" "$case_dir/updater/conf.fmpp"
cp "$test_dir/chconf_rt.h" "$case_dir/chconf.h"
cd "$case_dir/updater"

bash "$updater_dir/update_chconf_rt.sh" "$case_dir/chconf.h"
for setting in \
  'CH_CFG_ST_FREQUENCY 12345' \
  'CH_CFG_USE_RFCU FALSE' \
  'CH_DBG_ENABLE_ASSERTS TRUE' \
  'PORT_USE_SYSCALL TRUE' \
  'PORT_SWITCHED_REGIONS_NUMBER 1' \
  'PORT_INT_REQUIRED_STACK (32U + 16U)'; do
  name=${setting%% *}
  value=${setting#* }
  actual=$(sed -n "s/^#define $name  */$name /p" "$case_dir/chconf.h")
  test "$actual" = "$name $value"
done

cp "$case_dir/chconf.h" "$case_dir/first.h"
bash "$updater_dir/update_chconf_rt.sh" "$case_dir/chconf.h"
cmp "$case_dir/first.h" "$case_dir/chconf.h"

# A project without port overrides must still match the stock RT template.
cp "$repo_dir/os/rt/templates/chconf.h" "$case_dir/chconf.h"
bash "$updater_dir/update_chconf_rt.sh" "$case_dir/chconf.h"
cmp "$repo_dir/os/rt/templates/chconf.h" "$case_dir/chconf.h"

echo "RT configuration values, port overrides and regeneration stability: PASS"
