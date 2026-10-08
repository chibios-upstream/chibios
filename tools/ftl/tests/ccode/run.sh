#!/bin/sh
set -eu

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
repo_root=$(CDPATH= cd -- "$script_dir/../../../.." && pwd)
schema="$repo_root/tools/ftl/schema/ccode/modules.xsd"
work=$(mktemp -d "${TMPDIR:-/tmp}/ccode-tests.XXXXXX")
trap 'rm -rf "$work"' EXIT HUP INT TERM

xmllint --noout --schema "$schema" "$script_dir/semantic_types.xml"
mkdir "$work/out"
cat > "$work/config.fmpp" <<CONFIG
sourceRoot: $repo_root/tools/ftl/processors/ccode
outputRoot: out
dataRoot: $script_dir
modes: [execute(module.h.ftl), ignore(**)]
freemarkerLinks: { ftllibs: $repo_root/tools/ftl/libs }
data: { xml:xml(semantic_types.xml) }
CONFIG
(cd "$work" && fmpp -q -C config.fmpp)

cat > "$work/semantic_types_lld.h" <<'HEADER'
/* This type must already exist when the generated include is encountered. */
typedef semantic_value_t semantic_backend_t;
#define semantic_lld_fields semantic_backend_t backend
HEADER
cat > "$work/check.c" <<'SOURCE'
#include <stddef.h>
#include "semantic_types.h"
#if SEMANTIC_TYPES_ENABLED
_Static_assert(SEMANTIC_FIRST == 3, "explicit enum value");
_Static_assert(SEMANTIC_NEXT == 4, "implicit enum value");
_Static_assert(SEMANTIC_LAST == 8, "enum expression");
_Static_assert(SEMANTIC_FLAG == -1, "negative enum value");
_Static_assert(sizeof(enum semantic_kind) == sizeof(semantic_kind_t),
               "enum tag");
_Static_assert(sizeof(struct semantic_record) == sizeof(semantic_record_t),
               "struct tag");
_Static_assert(sizeof(union semantic_union) == sizeof(semantic_union_t),
               "union tag");
_Static_assert(offsetof(semantic_record_t, payload.pair.first) ==
               offsetof(semantic_record_t, payload.bytes), "nested union");
_Static_assert(offsetof(semantic_record_t, payload.pair.second) ==
               offsetof(semantic_record_t, payload.pair.first) +
               sizeof(semantic_value_t), "nested struct");
_Static_assert(sizeof(semantic_anonymous_t) == sizeof(semantic_record_t),
               "anonymous struct");
_Static_assert(sizeof(semantic_anonymous_union_t) == sizeof(semantic_record_t),
               "anonymous union");
int main(void) {
  semantic_record_t record = {0};
  record.payload.pair.first = 7;
  record.tail.value = record.payload.pair.first;
  record.backend = record.tail.value;
  return record.backend != 7;
}
#else
int main(void) {
  semantic_value_t value = 0;
  return (int)value;
}
#endif
SOURCE
for enabled in 0 1; do
  "${CC:-cc}" -std=c11 -Wall -Wextra -Werror -Wpedantic \
    -DSEMANTIC_TYPES_ENABLED="$enabled" -I"$work" -I"$work/out/include" \
    "$work/check.c" -o "$work/check"
  "$work/check"
done

# Verify documentation attachment and indentation, plus malformed XML rejection.
python3 - "$script_dir/semantic_types.xml" "$work" "$schema" <<'PY'
from pathlib import Path
import re
import subprocess
import sys
import xml.etree.ElementTree as ET

fixture, work, schema = sys.argv[1:]
work = Path(work)
header = (work / "out/include/semantic_types.h").read_text()
root = ET.parse(fixture)
for node in root.findall(".//brief"):
    assert node.text in header, node.text
assert re.search(r"@brief\s+Implicit successor value\.\s+\*/\s+SEMANTIC_NEXT,", header)
assert re.search(r"@brief\s+First pair member\.\s+\*/\s+semantic_value_t\s+first;", header)
assert re.search(r"^      semantic_value_t\s+first;$", header, re.M)
assert re.search(r"^    } pair;$", header, re.M)
assert re.search(r"@brief\s+Documented backend extension fields\.\s+\*/\s+semantic_lld_fields;", header)

for case in ("empty_enum", "unnamed_enumerator", "unnamed_macro", "untyped_field"):
    tree = ET.parse(fixture)
    if case == "empty_enum":
        enum = tree.find(".//enumtype")
        for item in list(enum):
            enum.remove(item)
    elif case == "unnamed_enumerator":
        del tree.find(".//enumerator").attrib["name"]
    elif case == "unnamed_macro":
        del tree.find(".//fieldmacro").attrib["name"]
    else:
        del tree.find(".//field").attrib["ctype"]
    invalid = work / (case + ".xml")
    tree.write(invalid)
    result = subprocess.run(["xmllint", "--noout", "--schema", schema, str(invalid)],
                            stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    assert result.returncode != 0, case
print("Semantic type generation, C layout, documentation and schema checks passed.")
PY
