#!/bin/sh
# Host test: patches/ go onto Tarrasch and engine-patches/ onto the Toy
# Engine at the commits tarrasch.lock pins, cleanly, and the Amiga branches
# are there. Needs the network to fetch them.
# Copyright (c) 2026 AmiCradle Team. MIT licence, like Tarrasch.
set -eu
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
. "$HERE/../tarrasch.lock"
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
get() { git -C "$T" init -q "$1" && git -C "$T/$1" fetch -q --depth 1 "$2" "$3" && git -C "$T/$1" checkout -q FETCH_HEAD; }
get tarrasch "$TARRASCH_GIT" "$TARRASCH_COMMIT"
get toy "$TOY_GIT" "$TOY_COMMIT"
fail=0
for p in "$HERE"/../patches/*.patch; do patch -s -d "$T/tarrasch" -p1 --dry-run < "$p" >/dev/null && patch -s -d "$T/tarrasch" -p1 < "$p" || { echo "FAIL: $p"; fail=1; }; done
for p in "$HERE"/../engine-patches/*.patch; do patch -s -d "$T/toy" -p1 < "$p" || { echo "FAIL: $p"; fail=1; }; done
has() { grep -q "$2" "$T/$1" || { echo "FAIL: $1 has no $2"; fail=1; }; }
has tarrasch/src/Portability.h 'define THC_AMIGA'
has tarrasch/src/UnixUciInterface.cpp 'tarrasch_engine_start'
has tarrasch/src/Appdefs.h 'DEFAULT_ENGINE "ToyEngine"'
has tarrasch/src/fseek64.h 'ifdef THC_AMIGA'
has tarrasch/src/Portability.h 'tarrasch_amiga_itoa.h'
has tarrasch/src/main.cpp 'tarrasch_amiga_snglinst.h'
has toy/src/toyengine.cpp 'toy_amiga_io.inc'
has toy/src/ChessEngine.cpp 'toy_amiga.h'
# no file is left half patched
[ -z "$(find "$T" -name '*.rej' -o -name '*.orig' | head -1)" ] || { echo "FAIL: rejects"; fail=1; }
[ $fail = 0 ] && echo "patches: all apply"
exit $fail
