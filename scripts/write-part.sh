#!/bin/sh
# Writes the Part file GFOpenUpTool INSTALL reads (gfopenup src/openuptool.c):
# one "copy" line per file under the part drawer, each to SYS:<its path>.
#   write-part.sh <part drawer> <version>
# Copyright (c) 2026 AmiCradle Team. MIT licence, like Tarrasch.
set -eu
P=$1; V=$2
{
    echo "part Tarrasch $V"
    echo "; Tarrasch: a chess GUI for playing, training and keeping games, with the Tarrasch Toy Engine"
    echo "needs 68040"
    (cd "$P" && find Utilities -type f ! -name Part | LC_ALL=C sort) | while read -r f; do
        echo "copy \"$f\" \"SYS:$f\""
    done
} > "$P/Part"
