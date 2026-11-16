#!/bin/sh
# Host test for scripts/write-part.sh: the Part file GFOpenUpTool reads.
# Copyright (c) 2026 AmiCradle Team. MIT licence, like Tarrasch.
set -eu
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
mkdir -p "$T/Utilities/Tarrasch/Engines"
: > "$T/Utilities/Tarrasch/Tarrasch"; : > "$T/Utilities/Tarrasch/Tarrasch.ini"; : > "$T/Utilities/Tarrasch/Engines/ToyEngine"
sh "$HERE/../scripts/write-part.sh" "$T" 3.20b
fail=0
expect() { grep -qxF "$1" "$T/Part" || { echo "FAIL: no line: $1"; fail=1; }; }
expect 'part Tarrasch 3.20b'
expect 'needs 68040'
expect 'copy "Utilities/Tarrasch/Tarrasch" "SYS:Utilities/Tarrasch/Tarrasch"'
expect 'copy "Utilities/Tarrasch/Tarrasch.ini" "SYS:Utilities/Tarrasch/Tarrasch.ini"'
expect 'copy "Utilities/Tarrasch/Engines/ToyEngine" "SYS:Utilities/Tarrasch/Engines/ToyEngine"'
[ "$(head -1 "$T/Part")" = 'part Tarrasch 3.20b' ] || { echo "FAIL: first line"; fail=1; }
[ "$(grep -c '^copy' "$T/Part")" = 3 ] || { echo "FAIL: copy count (the Part file itself must not be listed)"; fail=1; }
sh "$HERE/../scripts/write-part.sh" "$T" 3.20b   # a second run: same file
[ "$(grep -c '^copy' "$T/Part")" = 3 ] || { echo "FAIL: second run lists the Part file"; fail=1; }
[ $fail = 0 ] && echo "Part file: all checks passed"
exit $fail
