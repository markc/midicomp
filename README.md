# midicomp

##### v0.0.8 20170315 markc@renta.net (AGPL-3.0)

A program to manipulate SMF (Standard MIDI File) files. `midicomp` will
both read and write SMF files in 0 or format 1 and also read and write
its own plain text format. This means a SMF file can be turned into
easily parseable text, edited with any text editor or filtered through
any script language, and "recompiled" back into a binary SMF file.

* Copyright (C) 2003-2017 Mark Constable <markc@renta.net> 
* License: AGPL-3.0 - http://www.gnu.org/licenses/agpl.html
* Originally based on mf2t/t2fm by Piet van Oostrum

### To Build from Source
```
git clone https://github.com/markc/midicomp
mkdir midicomp/build
cd midicomp/build
cmake ..
make
sudo make install #(optional)
```
### Changes

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
