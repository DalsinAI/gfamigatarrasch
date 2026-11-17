# Tarrasch for GFOS

[Tarrasch](https://github.com/billforsternz/tarrasch-chess-gui), Bill Forster's
chess GUI for playing, training and keeping games, built for GFOS (AmigaOS
3.2.3 with GFOpenUp) on the GFOpenWX Toolkit, with the Tarrasch Toy Engine as
its engine.

- 68040, soft float with the FPU taken when there is one (fpuauto).
- Tarrasch and the Toy Engine are fetched at the commits `tarrasch.lock`
  pins and patched (`patches/`, `engine-patches/`); nothing third-party is
  kept here.
- The engine runs as its own AmigaDOS process, joined to Tarrasch by two
  `PIPE:` channels, in place of `fork()` and `pipe()`
  (`compat/tarrasch_amiga_engine.c`). Any UCI engine built for AmigaOS works:
  pick it in Options > Engine.
- The Toy Engine's Windows reader thread is an AmigaOS reader process
  (`compat/toy_amiga_io.inc`).

## Build

    STOVE=~/AmiCradle/stoves/os32-gcc16-open6 OUT=out ./build-amiga.sh

`out/Parts/Tarrasch` is a GFOpenUp part:

    GFOpenUpTool INSTALL Parts/Tarrasch

It puts Tarrasch in `SYS:Utilities/Tarrasch`, with `Tarrasch.ini` beside it
so its settings, opening book and databases stay in that drawer, and the Toy
Engine in `SYS:Utilities/Tarrasch/Engines`.

## Tests

    make -C tests test

Host tests for the engine link (ring, `PIPE:` names, command line, clock),
the Toy Engine's line splitter, the Part file, and that the patches apply.

## Licence

Tarrasch and the Toy Engine: MIT licence, Bill Forster. This port: MIT
licence, Copyright (c) 2026 AmiCradle Team.

Designed, tested and delivered by The AmiCradle Team.
