# midicomp

##### v0.1.0 20260613 markc@renta.net (MIT)

A program to manipulate SMF (Standard MIDI File) files. `midicomp` will
both read and write SMF files in 0 or format 1 and also read and write
its own plain text format. This means a SMF file can be turned into
easily parseable text, edited with any text editor or filtered through
any script language, and "recompiled" back into a binary SMF file.

* Copyright (C) 2003-2026 Mark Constable <markc@renta.net>
* License: MIT - see [LICENSE](LICENSE)
* Co-authored-by: Claude Code, Codex
* Originally based on mf2t/t2fm by Piet van Oostrum

### Download

A prebuilt Linux x86-64 binary is attached to the latest release:

* [midicomp-0.1.0-linux-x86_64](https://github.com/markc/midicomp/releases/download/v0.1.0/midicomp-0.1.0-linux-x86_64)
  ([sha256](https://github.com/markc/midicomp/releases/download/v0.1.0/midicomp-0.1.0-linux-x86_64.sha256))

See all releases at https://github.com/markc/midicomp/releases. For other
platforms, build from source below.

### To Build from Source
```
git clone https://github.com/markc/midicomp
mkdir midicomp/build
cd midicomp/build
cmake ..
make
sudo make install #(optional)
```
This is pre-ANSI K&R C, so the build pins the gnu89 standard; a modern
compiler's default would reject it. CMake 3.10+ is required.

### Running the Tests
A CTest suite round-trips the bundled `ex1.mid` sample through the decoder
and compiler:
```
cd build
ctest --output-on-failure
```

### Changes

* v0.1.0 20260613 Relicensed to MIT, fixed CMake 3.10+ build, added CTest suite, warning-clean -Wall build
* v0.0.8 20170315 Added unistd.h include to yyread.c
* v0.0.7 20120724 Added incremental option for time tags [Alexandre Oberlin]
* v0.0.6 20110727 Compile as a CMake project, remove QMake pro files
* v0.0.5 20101205 Set up to compile from Qt Creator
* v0.0.4 20080115 Unknown changes
* v0.0.2 20070722 Fixed gcc4+ compiler bug and exit warnings
* v0.0.1 20031129 Initial release, combined mf2t+t2fm, added getopt args

## Usage

    -d  --debug     send any debug output to stderr
    -v  --verbose   output in columns with notes on
    -c  --compile   compile ascii input into SMF
    -n  --note      note on/off value as note|octave
    -t  --time      use absolute time instead of ticks
    -fN --fold=N    fold sysex data at N columns

To translate a SMF file to plain ascii format

    midicomp some.mid                   # to view as plain text
    midicomp some.mid > some.asc        # to create a text version

To translate a plain ascii formatted file to SMF

    midicomp -c some.asc some.mid       # input and output filenames
    midicomp -c some.mid < some.asc     # input from stdin with one arg

    midicomp some.mid | somefilter | midicomp -c some2.mid

## Format of the textfile

    File header:            Mfile <format> <ntrks> <division>
    Start of track:         MTrk
    End of track:           TrkEnd

    Note On:                On <ch> <note> <vol>
    Note Off:               Off <ch> <note> <vol>
    Poly Pressure:          PoPr[PolyPr] <ch> <note> <val>
    Channel Pressure:       ChPr[ChanPr] <ch> <val>
    Controller parameter:   Par[Param] <ch> <con> <val>
    Pitch bend:             Pb <ch> <val>
    Program change:         PrCh[ProgCh] <ch> <prog>
    Sysex message:          SysEx <hex>
    Arbutrary midi bytes:   Arb <hex>

    Sequence nr:            Seqnr <num>
    Key signature:          KeySig <num> <manor>
    Tempo:                  Tempo <num>
    Time signature:         TimeSig <num>/<num> <num> <num>
    SMPTE event:            SMPTE <num> <num> <num> <num> <num>

    Meta text events:       Meta <texttype> <string>
    Meta end of track:      Meta TrkEnd
    Sequencer specific:     SeqSpec <type> <hex>
    Misc meta events:       Meta <type> <hex>

### The <> have the following meaning

    <ch>                    ch=<num>
    <note>                  n=<noteval>  [note=<noteval>]
    <vol>                   v=<num> [vol=<num>]
    <val>                   v=<num> [val=<num>]
    <con>                   c=<num> [con=<num>]
    <prog>                  p=<num> [prog=<num>]
    <manor>                 minor or major
    <noteval>               either a <num> or A-G optionally followed by #,
                            followed by <num> without intermediate spaces.
    <texttype>              Text Copyright SeqName TrkName InstrName Lyric Marker Cue
    <type>                  a hex number of the form 0xab
    <hex>                   a sequence of 2-digit hex numbers (without 0x)
                            separated by space
    <string>                a string between double quotes (like "text").

## Misc notes

Channel numbers are 1-based, all other numbers are as they appear in the
midifile.

`<division>` is either a positive number (giving the time resolution in
clicks per quarter note) or a negative number followed by a positive
number (giving SMPTE timing).

`<format> <ntrks> <num>` are decimal numbers.

The `<num>` in the Pb is the real value (two midibytes combined)

In Tempo it is a long (32 bits) value. Others are in the interval 0-127

The SysEx sequence contains the leading F0 and the trailing F7.

In a string certain characters are escaped:

" and \ are escaped with a \
a zero byte is written as \0
CR and LF are written as \r and \n respectively
other non-printable characters are written as `\x<2 hex digits>`
When -f is given long strings and long hex sequences are folded by inserting
`\<newline><tab>`. If in a string the next character would be a space or
tab it will be escaped by \

midicomp will accept all formats that mf2t can produce, plus a number of
others. Input is case insensitive (except in strings) and extra tabs and
spaces are allowed. Newlines are required but empty lines are allowed.
Comment starts with # at the beginning of a lexical item and continues
to the end of the line. The only other places where a # is legal are
insides strings and as a sharp symbol in a symbolic note.

In symbolic notes + and # are allowed for sharp, b and - for flat.

In bar:beat:click time the : may also be /

On input a string may also contain \t for a tab, and in a folded
string any whitespace at the beginning of a continuation line is skipped.

Hex sequences may be input without intervening spaces if each byte is
given as exactly 2 hex digits.

Hex sequences may be given where a string is required and vice versa.

Hex numbers of the form 0xaaa and decimal numbers are equivalent.
Also allowed as numbers are "bank numbers" of the form '123. In fact
this is equivalent to the octal number 012 (subtract 1 from each
digit, digits 1-8 allowed). The letters a-h may also be used for 1-8.

The input is checked for correctness but not extensively. An
errormessage will generally imply that the resulting midifile is illegal.

channel number can be recognized by the regular expression `/ch=/`.
note numbers by `/n=/` or `/note=/`, program numbers by `/p=/` or `/prog=/`.
Meta events by `/^Meta/` or `/^SeqSpec/`.
Text events by `/"/`, continued lines by `/\\$/`, continuation lines by `/$\t/`
(that was a TAB character).

## Examples

To convert a huge number of MID files try some variation of this command...

    find /path/to/MIDIs -type f -name '*.mid' | while read i; do midicomp $i > ${i}_output.txt; done

---

In awk each parameter is a field, in perl you can use split to get the
parameters (except for strings).

The following perl script changes note off messages to note on with
vol=0, deletes controller 3 changes, makes some note reassignments on
channel 10, and changes the channel numbers from channel 1 depending
on the track number.

    ------------------------------- test.pl -------------------------------
    %drum = (62, 36, 63, 47, 65, 61, 67, 40, 68, 54);

    while (<>) {
        next if /c=3/;
        s/Off(.*)v=[0-9]*/On\1v=0/;
        if (/ch=10/ && /n=([0-9]*)/ && $drum{$1}) {
            s/n=$1/"n=".$drum{$1}/e;
        }
        if (/^MTrk/) {++$trknr ; print "track $trknr\n";}
        if ($trknr > 2) { s/ch=1\b/ch=3/; }
        else  { s/ch=1\b/ch=4/; }
        print || die "Error: $!\n";
    }
    ------------------------------------------------------------------------

and this is the corresponding awk script.

    ------------------------------- test.awk -------------------------------
    BEGIN { drum[62] = 36; drum[63] = 47; drum[65] = 61; \
        drum[67] = 40; drum[68] = 54 }
    /c=3/ { next }
    ($2 == "Off") { $2 = "On"; $5 = "v=0" }
    /ch=10/ { n = substr($4, 3); if (n in drum) $4 = "n=" drum[n] }
    /^MTrk/ { trknr++ }
    /ch=1 / { if (trknr > 2) { $3 = "ch=2" } else { $3 = "ch=3" } }
    { print }
    ------------------------------------------------------------------------

## Building for Other Platforms

The published binary is Linux x86-64. Because `midicomp` is plain portable C
with a CMake build, other targets are straightforward — the catch is that you
need to build *on* (or *for*) each target OS. Below are recipes for anyone
adventurous enough to want macOS or Windows builds.

### macOS (Apple Silicon / arm64) via GitHub Actions

You can't cross-compile a macOS binary from Linux without an Apple SDK, but
GitHub's `macos-14` runners are real Apple Silicon machines, so CI can build
and test natively and attach the result to a release. Drop this in
`.github/workflows/release-macos.yml`:

```yaml
name: release-macos-arm64
on:
  release:
    types: [published]
  workflow_dispatch:        # also runnable by hand from the Actions tab

permissions:
  contents: write           # needed to upload release assets

jobs:
  build:
    runs-on: macos-14       # Apple Silicon (arm64)
    steps:
      - uses: actions/checkout@v4
      - name: Configure & build
        run: |
          cmake -B build -DCMAKE_BUILD_TYPE=Release
          cmake --build build
      - name: Test
        run: ctest --test-dir build --output-on-failure
      - name: Package
        run: |
          strip build/midicomp
          mv build/midicomp midicomp-macos-arm64
          shasum -a 256 midicomp-macos-arm64 > midicomp-macos-arm64.sha256
      - name: Upload to the release
        if: github.event_name == 'release'
        run: gh release upload "${{ github.event.release.tag_name }}" \
               midicomp-macos-arm64 midicomp-macos-arm64.sha256
        env:
          GH_TOKEN: ${{ github.token }}
```

Publish a release (or trigger the workflow manually) and the arm64 binary shows
up as an asset. Note it is **unsigned / un-notarized** — on first run macOS
Gatekeeper will block it; clear the quarantine flag with
`xattr -d com.apple.quarantine ./midicomp-macos-arm64`. For an `x86_64` build
swap the runner to `macos-13`, or build a universal binary on `macos-14` with
`-DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"`.

### Windows 10/11

The cleanest native `.exe` comes from **MSYS2 + MinGW-w64** (UCRT), which gives a
standalone binary with no runtime DLL dependency beyond the OS. From an MSYS2
"UCRT64" shell:

```sh
pacman -S --needed mingw-w64-ucrt-x86_64-toolchain mingw-w64-ucrt-x86_64-cmake
cmake -B build -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_EXE_LINKER_FLAGS="-static"
cmake --build build      # produces build/midicomp.exe
```

Two portability notes for the MinGW build:

- `yyread.c` uses the legacy BSD `bcopy()`, which MinGW does not provide. Swap it
  for the standard `memmove()` (same arguments, source and dest reversed:
  `memmove(dest, src, n)`), which is the modern equivalent.
- `getopt_long()` is a GNU extension; MinGW-w64 ships it, so the option parsing
  works as-is. **MSVC (cl.exe) does not** provide `getopt_long`, `unistd.h`, or
  POSIX `read()`, so a native MSVC build would need those shimmed — MinGW is much
  less work.

This can also be automated on a `windows-latest` GitHub Actions runner using the
[`msys2/setup-msys2`](https://github.com/msys2/setup-msys2) action with the same
`pacman` + `cmake` steps, uploading `midicomp.exe` the same way as the macOS job
above.

Alternatively, **WSL** (Windows Subsystem for Linux) runs the Linux binary
unmodified — easiest of all if a true native `.exe` isn't required.
