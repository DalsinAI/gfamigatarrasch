/* A Shell gives 4 KB of stack; libnix's startup sets this much up instead.
 * wxWidgets' event handling and Tarrasch's move tree and PGN reader nest deeply.
 * Copyright (c) 2026 AmiCradle Team. MIT licence, like Tarrasch. */
unsigned long __stack = 512 * 1024;
static const char version[] __attribute__((used)) = "$VER: Tarrasch 3.20b Amiga (16.11.2026)";
