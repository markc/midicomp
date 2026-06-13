# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

`midicomp` is a single C executable that converts Standard MIDI Files (SMF, format 0/1)
to and from a line-based plain-text format. The text format is designed to be parsed,
edited, or piped through scripts, then "recompiled" back into a binary `.mid` file:

```
midicomp some.mid | somefilter | midicomp -c some2.mid
```

It descends from Piet van Oostrum's `mf2t`/`t2mf`, combined into one binary.

## Build

CMake is the canonical build (the `build/` dir is gitignored):

```
mkdir build && cd build
cmake ..
make
sudo make install   # optional, installs to <prefix>/bin
```

`Makefile.orig` is the original upstream flex-based makefile, kept for reference only —
it regenerates `t2mflex.c` from `t2mf.fl`.

### Tests

A CTest suite lives in `tests/run_test.cmake` (a pure-CMake driver — no shell, bash,
or other interpreter needed, so it runs anywhere CMake does). After building, run:

```
cd build && ctest --output-on-failure
```

Four checks, all using the committed `ex1.mid` sample and its golden text files:

- **plain** — `midicomp ex1.mid` byte-matches `ex1-plain.txt`.
- **verbose** — `midicomp -v -t ex1.mid` byte-matches `ex1-verbose.txt`. Note the
  `-t`: `ex1-verbose.txt` uses absolute `mmm:bb:ttt` time, which is `-v` **and** `-t`,
  not `-v` alone.
- **roundtrip** — decode → `-c` recompile → decode reproduces the first decode
  (the text format is idempotent).
- **canonical** — midicomp's own SMF output re-compiles byte-for-byte identically.
  Note: a recompiled `ex1.mid` is NOT byte-identical to the original, because the
  original uses MIDI running status (omitted repeated status bytes) which midicomp
  expands to explicit status on write. Byte-stability only holds from gen-1 onward.

This is pre-ANSI K&R C (empty-parens prototypes, implicit declarations) and will NOT
compile under a modern compiler's default C standard. `CMakeLists.txt` pins
`CMAKE_C_STANDARD 90` with extensions on to handle this. If you compile the sources
directly without CMake, pass the gnu89 standard explicitly:

```
cc -std=gnu89 -w -o midicomp yyread.c midicomp.c t2mflex.c -I.
```

## Architecture

The program has two independent data paths wired through a callback-based MIDI library
(the function-pointer hooks declared in `midifile.h`):

**SMF → text (decode), the default direction.** `mfread()` (midicomp.c) parses the
binary `.mid` byte stream and dispatches each event to a `Mf_*` function pointer. The
pointers are bound in `initfuncs()` to the `my*` printer functions (`mynon`, `mytempo`,
`mymtext`, etc.) which emit the plain-text format. Output style is controlled by globals
set from CLI flags: `verbose`, `notes` (note|octave names via `mknote`), `times`
(absolute vs. tick deltas), `fold` (sysex hex wrapping).

**text → SMF (encode), the `-c`/`--compile` direction.** `translate()` calls
`mfwrite()`, which calls back into `mywritetrack()`. That function pulls tokens from the
**flex lexer** (`t2mflex.c`, generated from `t2mf.fl`) — `yylex()` returns token codes
(`MTHD`, `ON`, `META`, `INT`, …) defined in `t2mf.h`/`midicomp.h`. The `check*`/`get*`
helpers (`checkchan`, `getint`, `gethex`, `splitval`) consume the token stream and the
`mf_w_*` writers emit MIDI bytes with proper variable-length encoding (`WriteVarLen`).

`main()` (midicomp.c) does getopt_long parsing and picks the direction. `yyread.c`
provides `_yyread`, a CR-stripping read used by the lexer so DOS line endings parse.

## Editing the grammar

`t2mflex.c` is a **generated** flex output but is committed, so a normal build does NOT
need flex installed. Only regenerate it when changing the text grammar in `t2mf.fl`:

```
flex -i -s -Ce t2mf.fl && mv lex.yy.c t2mflex.c
```

The token IDs in `t2mf.h` and `midicomp.h` must stay in sync with what the lexer
returns and what `mywritetrack()` expects.

## Gotchas

- The `VERSION` file (0.0.5) lags the README/source version (0.0.8) — README is
  authoritative for the user-facing version.
- Two near-duplicate token-definition headers exist: `midicomp.h` (current) and
  `t2mf.h` (legacy, included by `t2mf.fl`). Keep both consistent if you touch token IDs.
- The MIDI event names map to status bytes via `#define`s (e.g. `ON` = `note_on` =
  `0x90`) in `midicomp.h` — these are used as both lexer token codes and protocol bytes.
