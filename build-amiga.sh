#!/bin/sh
# Builds Tarrasch (the chess GUI) and its Toy Engine for GFOS (AmigaOS 3.2.3
# with GFOpenUp, on the GFOpenKick ROM or a Kickstart) with a stove: 68040,
# soft float with the FPU taken when there is one (fpuauto), on the GFOpenWX
# Toolkit.
#   STOVE=.../stoves/os32-gcc16-open6 OUT=out ./build-amiga.sh
#   (WXT_SRC=... to use a toolkit tree already on disk)
# Tarrasch, the Toy Engine, the toolkit and fpuauto are fetched at the
# commits tarrasch.lock gives; nothing third-party is kept here. The build
# tree is kept, so a run that stops at BUDGET_TOTAL seconds goes on next time.
# OUT gets Parts/Tarrasch: a GFOpenUp part (its Part file and the files it
# puts in SYS:Utilities/Tarrasch), installed with
#   GFOpenUpTool INSTALL Parts/Tarrasch
# Copyright (c) 2026 AmiCradle Team. MIT licence, like Tarrasch.
set -eu
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
. "$HERE/tarrasch.lock"
STOVE=${STOVE:-"$HOME/AmiCradle/stoves/os32-gcc16-open6"}
# outside the checkout: Cradle cleans it (git clean -ffdx) before every job
WORK="${WORK:-$HOME/.cache/gfamigatarrasch/build}"; OUT="${OUT:-$WORK/out}"; JOBS=${JOBS:-12}
TC="$STOVE/prefix"; PATH="$TC/bin:$PATH"; export PATH
mkdir -p "$WORK/deps" "$OUT"; OUT=$(cd "$OUT" && pwd)
t0=$(date +%s)
BUDGET=${BUDGET_TOTAL:-840}
left() { echo $(( BUDGET - ($(date +%s) - t0) )); }

# a repository at one commit, fetched once (SPARSE: only those folders)
fetch() {
    d="$WORK/deps/$1"
    if [ "$(git -C "$d" rev-parse HEAD 2>/dev/null)" != "$3" ]; then
        rm -rf "${d:?}"; mkdir -p "$d"
        git -C "$d" init -q
        if [ -n "${4:-}" ]; then
            git -C "$d" config core.sparseCheckout true
            echo "$4" > "$d/.git/info/sparse-checkout"
            git -C "$d" fetch -q --depth 1 --filter=blob:none "$2" "$3"
        else
            git -C "$d" fetch -q --depth 1 "$2" "$3"
        fi
        git -C "$d" checkout -q FETCH_HEAD
        [ "$(git -C "$d" rev-parse HEAD)" = "$3" ] || { echo "$1: commit mismatch"; exit 1; }
    fi
}

# a source tree at its pinned commit with a folder of patches on it
patched() {   # name git commit patchdir
    s="$WORK/$1"
    psum=$(cat "$4"/*.patch | sha256sum | cut -c1-16)
    if [ "$(cat "$s/.patched" 2>/dev/null)" != "$3-$psum" ]; then
        rm -rf "$s"; mkdir -p "$s"
        git -C "$s" init -q
        git -C "$s" fetch -q --depth 1 "$2" "$3"
        git -C "$s" checkout -q FETCH_HEAD
        [ "$(git -C "$s" rev-parse HEAD)" = "$3" ] || { echo "$1: commit mismatch"; exit 1; }
        for p in "$4"/*.patch; do
            patch -s -d "$s" -p1 < "$p" || { echo "patch failed: $p"; exit 1; }
        done
        echo "$3-$psum" > "$s/.patched"
        rm -rf "$WORK/obj-$1"
    fi
}

# --- the toolkit's SDK, again only when its tree changed ---------------------
if [ -z "${WXT_SRC:-}" ]; then fetch gfopenwxtoolkit "$WXT_REPO" "$WXT_COMMIT"; WXT_SRC="$WORK/deps/gfopenwxtoolkit"; fi
WXB="$WORK/gfopenwx"
WXREV=$( (git -C "$WXT_SRC" rev-parse HEAD 2>/dev/null; git -C "$WXT_SRC" diff HEAD 2>/dev/null) | sha256sum | cut -c1-16)
if [ ! -x "$WXB/out/sdk/bin/wx-config" ] || [ "$(cat "$WXB/out/.rev" 2>/dev/null)" != "$WXREV" ]; then
    # stopped here, before Cradle's own cap, so the log says how far it got
    set +e
    STOVE="$STOVE" WORK="$WXB" OUT="$WXB/out" JOBS=$JOBS timeout "$BUDGET" sh "$WXT_SRC/build-amiga.sh" > "$WORK.wx.log" 2>&1
    rc=$?
    set -e
    if [ "$rc" = 124 ]; then
        echo "toolkit: out of time after $(( $(date +%s) - t0 )) s, run again to go on"
        echo "  base objects: $(find "$WXB/base" -name '*.o' 2>/dev/null | wc -l), GUI objects: $(find "$WXB/gui/obj" -name '*.o' 2>/dev/null | wc -l)"
        tail -5 "$WORK.wx.log"
        exit 0
    fi
    [ "$rc" = 0 ] || { tail -40 "$WORK.wx.log"; exit 1; }
    [ -x "$WXB/out/sdk/bin/wx-config" ] || { tail -40 "$WORK.wx.log"; echo "no toolkit SDK"; exit 1; }
    echo "$WXREV" > "$WXB/out/.rev"
fi
echo "toolkit: $(( $(date +%s) - t0 )) s"
WXC="$WXB/out/sdk/bin/wx-config"

# --- fpuauto: libfpuauto.a and its link specs ---------------------------------
fetch fpuauto "$FPA_REPO" "$FPA_COMMIT" "/common/fpuauto/"
FA="$WORK/deps/fpuauto/common/fpuauto"; FAL="$WORK/fpuauto"
if [ ! -f "$FAL/libfpuauto.a" ] || [ "$(cat "$FAL/.rev" 2>/dev/null)" != "$FPA_COMMIT" ]; then
    rm -rf "$FAL"; mkdir -p "$FAL/obj"
    m68k-amigaos-gcc -m68040 -DFPUAUTO_COMMON -c "$FA/fpuauto.S" -o "$FAL/obj/fa_common.o"
    for n in $(sed 's/^--wrap=//' "$FA/fpuauto.wraps"); do
        m68k-amigaos-gcc -m68040 -DR_$n -c "$FA/fpuauto.S" -o "$FAL/obj/fa_$n.o"
    done
    m68k-amigaos-ar rcs "$FAL/libfpuauto.a" "$FAL"/obj/fa_*.o
    echo "$FPA_COMMIT" > "$FAL/.rev"
fi
LINK="-noixemul -m68040 -specs=$HERE/scripts/m68k-soft-m68040.specs -specs=$FA/fpuauto.specs -L$FAL"

# --- sources at their pinned commits, with patches/ and engine-patches/ ------
patched tarrasch "$TARRASCH_GIT" "$TARRASCH_COMMIT" "$HERE/patches"
patched toy "$TOY_GIT" "$TOY_COMMIT" "$HERE/engine-patches"
SRC="$WORK/tarrasch"; TOY="$WORK/toy"

# objects made against another toolkit commit go: its headers are not in
# the compile rules, and stale objects crash at start (gfamigagolly #3)
if [ "$(cat "$WORK/.toolkit-objs" 2>/dev/null)" != "$WXT_COMMIT" ]; then
    rm -rf "$WORK/obj-tarrasch"
    echo "$WXT_COMMIT" > "$WORK/.toolkit-objs"
fi

LATE=""
if m68k-amigaos-gcc -fno-late-combine-instructions -S -o /dev/null -x c /dev/null 2>/dev/null; then LATE="-fno-late-combine-instructions"; fi
CF="-noixemul -m68040 -O2 -fno-delete-null-pointer-checks $LATE -DNDEBUG"

# compile each listed source to obj/<name>.o once; FLAGS in $CC_FLAGS.
# Objects already made stay, so a run cut short goes on where it stopped.
compile_all() {   # objdir compiler srcdir files...
    od=$1; cc=$2; sd=$3; shift 3
    mkdir -p "$od"
    for f in "$@"; do
        o="$od/$(basename "$f" .cpp).o"
        [ -f "$o" ] || echo "$f"
    done > "$od/todo"
    [ -s "$od/todo" ] || return 0
    l=$(left)
    [ "$l" -gt 60 ] || { echo "out of time: run again to go on ($(wc -l < "$od/todo") files left)"; exit 0; }
    set +e
    # shellcheck disable=SC2016
    timeout "$l" xargs -P "$JOBS" -I{} sh -c '
        n=$(basename "$1" .cpp)
        nice -n 19 '"$cc"' $CC_FLAGS -c "'"$sd"'/$1" -o "'"$od"'/$n.o.tmp" > "'"$od"'/$n.log" 2>&1 \
            && mv "'"$od"'/$n.o.tmp" "'"$od"'/$n.o"' _ {} < "$od/todo"
    rc=$?
    set -e
    for f in $(cat "$od/todo"); do
        n=$(basename "$f" .cpp)
        [ -f "$od/$n.o" ] || { grep -E 'error' "$od/$n.log" 2>/dev/null | head -20; }
    done
    if [ "$rc" = 124 ]; then
        echo "out of time: run again to go on; not made yet:"
        for f in $(cat "$od/todo"); do [ -f "$od/$(basename "$f" .cpp).o" ] || echo "  $f"; done
        exit 0
    fi
    [ "$rc" = 0 ] || { echo "compile failed"; exit 1; }
}

# --- the Toy Engine: three units, no toolkit ---------------------------------
# (the stove's C++ library takes its locks and once-only setup from pthread,
# so the engine links it like every C++ program here)
CC_FLAGS="$CF -std=gnu++17 -include $HERE/compat/toy_amiga.h -I$HERE/compat -I$TOY/src -Wno-write-strings -Wno-format-security"
export CC_FLAGS
compile_all "$WORK/obj-toy" m68k-amigaos-g++ "$TOY/src" thc.cpp ChessEngine.cpp toyengine.cpp
# shellcheck disable=SC2086
m68k-amigaos-g++ $LINK -o "$WORK/obj-toy/ToyEngine.debug" \
    "$WORK/obj-toy/thc.o" "$WORK/obj-toy/ChessEngine.o" "$WORK/obj-toy/toyengine.o" \
    -lpthread > "$WORK/obj-toy/link.log" 2>&1 || { head -60 "$WORK/obj-toy/link.log"; exit 1; }
echo "toy engine: $(( $(date +%s) - t0 )) s"

# --- Tarrasch: every .cpp of src/ but the Windows ones and the test program --
FILES=$(cd "$SRC/src" && ls ./*.cpp | sed 's|^\./||' | grep -vxE 'legal-move-generator.cpp|tabartmsw.cpp|WinUciInterface.cpp|tarrasch-test.cpp')
CC_FLAGS="$CF -std=gnu++17 $("$WXC" --cxxflags) -I$HERE/compat -I$SRC/src -Wno-deprecated-declarations -Wno-write-strings -Wno-format-security"
export CC_FLAGS
# shellcheck disable=SC2086
compile_all "$WORK/obj-tarrasch" m68k-amigaos-g++ "$SRC/src" $FILES
O="$WORK/obj-tarrasch"
m68k-amigaos-gcc $CF -I"$HERE/compat" -c "$HERE/compat/tarrasch_amiga_engine.c" -o "$O/tarrasch_amiga_engine.o"
m68k-amigaos-gcc $CF -c "$HERE/compat/amiga_stack.c" -o "$O/amiga_stack.o"
echo "compile: $(( $(date +%s) - t0 )) s"

# --- the link: soft-float specs, then fpuauto's ------------------------------
OBJS=""; for f in $FILES; do OBJS="$OBJS $O/$(basename "$f" .cpp).o"; done
# shellcheck disable=SC2046,SC2086
m68k-amigaos-g++ $LINK -o "$O/Tarrasch.debug" $OBJS "$O/tarrasch_amiga_engine.o" "$O/amiga_stack.o" \
    $("$WXC" --libs std,richtext,aui) > "$O/link.log" 2>&1 || { head -60 "$O/link.log"; exit 1; }

# --- the GFOpenUp part: Parts/Tarrasch ----------------------------------------
P="$OUT/Parts/Tarrasch"; D="$P/Utilities/Tarrasch"; rm -rf "$P"; mkdir -p "$D/Engines"
m68k-amigaos-strip -o "$D/Tarrasch" "$O/Tarrasch.debug"
m68k-amigaos-strip -o "$D/Engines/ToyEngine" "$WORK/obj-toy/ToyEngine.debug"
# Tarrasch.ini beside the program: Tarrasch keeps its settings, book and
# databases in its own drawer (Repository::SetDirectories)
cp "$SRC/install/Tarrasch.ini" "$SRC/install/book.pgn" "$SRC/install/readme.txt" "$D/"
cp "$SRC/LICENSE" "$D/LICENSE.txt"
cp "$TOY/LICENSE" "$D/Engines/ToyEngine-LICENSE.txt"
sh "$HERE/scripts/write-part.sh" "$P" "$TARRASCH_VERSION"
ls -l "$D/Tarrasch" "$D/Engines/ToyEngine"
echo "part: $(grep -c '^copy' "$P/Part") files, $(du -sk "$P" | cut -f1) KB"
echo "fpuauto: $(m68k-amigaos-nm "$O/Tarrasch.debug" | grep -c ' T ___wrap_') wrapped routines"
echo "total: $(( $(date +%s) - t0 )) s"
