# 2026-06-13 — v0.2.0 security audit & hardening

A comprehensive security + functional audit of midicomp, driven by a multi-agent
workflow, triaged and reviewed with Codex, then released as v0.2.0. midicomp parses
untrusted input two ways — `midicomp evil.mid` (binary decode) and
`midicomp -c evil.txt out.mid` (text compile) — and an attacker fully controls those
bytes. This was the threat model throughout.

## Process

1. **Audit workflow** — 5 parallel finder agents (one per dimension: SMF decode,
   text compile, memory safety, correctness, CLI/robustness) → dedup → adversarial
   per-finding verification. 42 raw findings, 31 confirmed, 11 rejected (the rejected
   ones mostly hinged on a wrong assumption about `error()` — see below).
2. **Codex triage** — design decisions (not Mark's): `error()` semantics, `-c -`
   behaviour, running-status feature scope, and sanity-checking the fix approach.
3. **Implement** — security-critical fixes first, grounded in the audit + my own
   reading of the parser.
4. **Codex-review loop** — 3 rounds. Codex caught a false-pass test, a SMPTE
   regression I introduced, and a `bankno()` overflow I'd missed, before signing off.
5. **Release** — v0.2.0, sanitizer-clean, tagged, GitHub release with binary.

## The linchpin: `error()` was never defined

`error()` was declared `void error()` but **never defined** in the project, so it
linked to glibc's `error(int status, int errnum, const char *fmt, ...)`. Every one of
~33 compile-path `error("msg")` calls passed a string where glibc expected
`(status, errnum, format)` — undefined behaviour, and whether it aborted depended on
the low bits of the literal's address. Range checks therefore failed to abort, so
out-of-range bytes got written and divide-by-zero / NULL-deref paths became reachable.
This single bug was the root cause behind ~6 findings.

Fix (per Codex): rename to a project-local `mc_error()` (recoverable — print, resync
to EOL, `longjmp(erjump)` if inside a track else `exit(1)`), routed via
`#define error mc_error` in `midicomp.h`. The **lexer** maps `error` → `fatal()`
instead (via `t2mf.h`), because lexer errors fire inside `yylex()` and mc_error itself
calls `yylex()` (reentrancy). `bankno()` uses `fatal()` for the same reason.

## Memory-safety fixes (decode path)

- **OOB / NULL read in `metaevent()`** — read `m[0..4]` for fixed meta events (tempo,
  SMPTE, time/key sig, seqnr) without checking the payload length; with `Msgbuff`
  still NULL for a zero-length event this was a NULL deref (confirmed SIGSEGV under
  ASan). New `metafield()` bounds-checks and zero-pads. *This is why the bundled
  `ex1.mid` — whose TimeSig is only 2 bytes — used to print `TimeSig 4/4 32 99`: the
  `32 99` were stale heap bytes. It now prints the deterministic `4/4 0 0`; the golden
  fixtures were regenerated.*
- **`readtrack()` meta/sysex read loop** — `lookfor = Mf_toberead - readvarinum()`
  depended on unspecified C evaluation order and was off-by-one. Replaced with
  deterministic length-counted reads clamped to the remaining track bytes.
- **`readvarinum()`** — capped at 4 bytes (28-bit) and rejects an overlong VLQ rather
  than shifting `long` past its width (UB) or leaving stray continuation bytes.
- **Divide-by-zero (SIGFPE)** — guarded `Beat`/`Measure`/`denom` in `prtime()`,
  `mytimesig()` and the compile TimeSig path; a zero MThd division no longer crashes.

## Robustness / overflow fixes (compile path)

- `mf_w_sysex_event()` guards `size==0` before dereferencing.
- Lexer integer rules now use `strtol`/`strtoul` (defined clamping) instead of
  `sscanf("%ld"/"%lx")` (overflow UB). Lexer regenerated with flex 2.6.4 +
  `%option noyywrap`.
- `translate()` parses PPQ vs SMPTE division separately and validates the SMPTE
  resolution operand, bounding the value to 16 bits (prevents `4*Clicks/denom` int
  overflow) — while still accepting valid SMPTE (the first pass wrongly rejected it).
- Parsed time components bounded to the 28-bit range before the measure/beat
  multiplications; TimeSig numerator bounded to 1..255.
- `bankno()` (`$…` bank notation) guards `res*8+c` against signed overflow.
- `gethex()` validates hex bytes to 0..255 and treats OOM as fatal.

## Other

- Portability: `bcopy` → `memmove` in `yyread.c` (MinGW/Windows lacks bcopy).
- CLI: `-c -`/`-` mean stdout/stdin; `-f` validated with `strtol`+`errno`+`INT_MAX`;
  removed a stray `fprintf(stderr,"fold=%d")` debug line; fixed the decode-path
  "Output file error" message (it's the input file).

## Verification

- Zero-warning `-Wall` CMake build; 6 CTests pass (added `smpte` round-trip and a
  `security` mode replaying `tests/fixtures/`).
- AddressSanitizer + UndefinedBehaviorSanitizer: the original crash PoCs now exit
  cleanly; a truncation + byte-flip decode sweep and compile overflow/SMPTE/time
  sweeps (~1200 runs total across iterations) — 0 sanitizer hits.

## Decisions deferred to Codex (not Mark)

- `error()` → recoverable mc_error + fatal split, project-local name to avoid the
  glibc collision. (Codex)
- `-c -` → stdout (works to a file/redirect; the existing fseek check makes it error
  cleanly on a pipe). (Codex)
- Running-status output flag → **out of scope** for a security release; output stays
  canonical. (Codex)
- Handling short fixed meta events → zero-pad (deterministic, preserves the event,
  round-trips) rather than drop. (my call, accepted in review)
