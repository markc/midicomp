/***
# midicomp

A MIDI Compiler - convert SMF MIDI files to and from plain text.
***/

char *usage = "\
midicomp v0.0.6 20110727 markc@renta.net (AGPLv3) \n\
\n\
http://github.com/markc/midicomp \n\
\n\
Command line argument usage: \n\
\n\
  -d  --debug     send any debug output to stderr \n\
  -v  --verbose   output in columns with notes on \n\
  -c  --compile   compile ascii input into SMF \n\
  -n  --note      note on/off value as note|octave \n\
  -t  --time      use absolute time instead of ticks \n\
  -i  --inc       write/read incremental time or tick values to/from ascii file \n\
  -fN --fold=N    fold sysex data at N columns \n\
\n\
To translate a SMF file to plain ascii format: \n\
\n\
  midicomp some.mid               # to view as plain text \n\
  midicomp some.mid > some.asc    # to create a text version \n\
\n\
To translate a plain ascii formatted file to SMF: \n\
\n\
  midicomp -c some.asc some.mid   # input and output filenames \n\
  midicomp -c some.mid < some.asc # input from stdin with one arg \n\
\n\
  midicomp some.mid | somefilter | midicomp -c some2.mid \n";

#include <malloc.h>
#include <setjmp.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include "midicomp.h"

int main(int argc, char **argv) {

  FILE *efopen();
  Mf_nomerge = 1;
  opterr = 0;
  int compile = 0;
  int c;

  struct option long_options[] = {
    {"debug",   no_argument,     0, 'd'},
    {"verbose", no_argument,     0, 'v'},
    {"compile", no_argument,     0, 'c'},
    {"note",  no_argument,     0, 'n'},
    {"time",  no_argument,     0, 't'},
    {"inc",     no_argument,       0, 'i'},
    {"fold",  required_argument, 0, 'f'},
    {0, 0, 0, 0}
  };
  int option_index = 0;

  while ((c = getopt_long(argc, argv, "dvcntif:", long_options, &option_index)) != -1) {
    switch (c) {
    case 0:
      if (long_options[option_index].flag != 0)
        break;
    case 'd':
      dbg++;
      break;
    case 'f':
      fold = atoi(optarg);
      fprintf(stderr, "fold=%d\n", fold);
      break;
    case 'm':
      Mf_nomerge = 0;
      break;
    case 'n':
      notes++;
      break;
    case 't':
      times++;
      break;
    case 'i':
      incs++;
      break;
     case 'c':
      compile++;
      break;
     case 'v':
      verbose++;
      notes++;
      break;
     case 'h':
     default:
      fprintf(stderr, "%s\n", usage);
      return 1;
    }
  }

  if (dbg) fprintf(stderr, "main()\n");

  TrkNr = 0;
  Measure = 4;
  Beat = 96;
  Clicks = 96;
  M0 = 0;
  T0 = 0;

  if (compile) {
    char *infile;
    char *outfile;

    if (optind < argc) {
      yyin = efopen (argv[optind], "r");
      infile = argv[optind];
    } else {
      yyin = stdin;
      infile = "stdin";
    }
    if (optind+1 < argc ) {
      F = efopen(argv[optind+1], "wb");
      outfile = argv[optind+1];
    } else {
#ifdef SETMODE
      setmode (fileno(stdout), O_BINARY);
      F = stdout;
#else
      F = fdopen (fileno(stdout), "wb");
#endif
      outfile = "stdout";
    }

if (dbg) fprintf(stderr, "Compiling %s to %s\n", infile, outfile);

    Mf_putc = fileputc;
    Mf_wtrack = mywritetrack;
    translate();
    fclose(F);
    fclose(yyin);
  } else {
    if (verbose) {
      Onmsg   = "On      ch=%-2d  note=%-3s  vol=%-3d\n";
      Offmsg  = "Off     ch=%-2d  note=%-3s  vol=%-3d\n";
      PoPrmsg = "PolyPr  ch=%-2d  note=%-3s  val=%-3d\n";
      Parmsg  = "Param   ch=%-2d  con=%-3d   val=%-3d\n";
      Pbmsg   = "Pb      ch=%-2d  val=%-3d\n";
      PrChmsg = "ProgCh  ch=%-2d  prog=%-3d\n";
      ChPrmsg = "ChanPr  ch=%-2d  val=%-3d\n";
    }
    if (optind < argc)
      F = efopen(argv[optind], "rb");
    else
      F = fdopen(fileno(stdin), "rb");

    initfuncs();
    Mf_getc = filegetc;
    mfread();
    if (ferror(F)) error ("Output file error");
    fclose(F);
  }
}

void mfread() {

  if (Mf_getc == NULLFUNC)
    mferror("mfread() called without setting Mf_getc");

  readheader();
  while(readtrack()) ;
}

static int readmt(char *s) {

  int n = 0;
  char *p = s;
  int c;

  while ( n++<4 && (c=(*Mf_getc)()) != EOF ) {
    if ( c != *p++ ) {
      char buff[32];
      (void) strcpy(buff, "expecting ");
      (void) strcat(buff, s);
      mferror(buff);
    }
  }
  return(c);
}

static int egetc() {

  int c = (*Mf_getc)();

  if ( c == EOF )
    mferror("premature EOF");
  Mf_toberead--;
  return(c);
}

static void readheader() {

  int format, ntrks, division;

  if (readmt("MThd") == EOF) return;
  Mf_toberead = read32bit();
  format      = read16bit();
  ntrks       = read16bit();
  division    = read16bit();
  if (Mf_header) (*Mf_header)(format, ntrks, division);
  while(Mf_toberead > 0) (void) egetc();
}

static int readtrack() {

  long lookfor;
  int c, c1, type;
  int sysexcontinue = 0;
  int running = 0;
  int status = 0;
  int needed;
  static int chantype[] = {
    0, 0, 0, 0, 0, 0, 0, 0,
    2, 2, 2, 2, 1, 1, 2, 0
  };

  if (readmt("MTrk") == EOF) return(0);
  Mf_toberead = read32bit();
  Mf_currtime = 0;
  old_Mf_currtime = 0;
  if (Mf_starttrack) (*Mf_starttrack)();

  while (Mf_toberead > 0) {
    old_Mf_currtime=Mf_currtime;
    Mf_currtime += readvarinum();
    c = egetc();
    if (sysexcontinue && c != 0xf7)
      mferror("didn't find expected continuation of a sysex");
    if ((c & 0x80) == 0) {
      if (status == 0) mferror("unexpected running status");
      running = 1;
      c1 = c;
      c = status;
    } else if (c < 0xf0) {
      status = c;
      running = 0;
    }
    needed = chantype[ (c>>4) & 0xf ];
    if (needed) {
      if (!running) c1 = egetc();
      chanmessage(status, c1, (needed>1) ? egetc() : 0 );
      continue;
    }

    switch(c) {
     case 0xff:
      type = egetc();
      lookfor = Mf_toberead - readvarinum();
      msginit();
      while(Mf_toberead >= lookfor) msgadd(egetc());
      metaevent(type);
      break;
     case 0xf0:
      lookfor = Mf_toberead - readvarinum();
      msginit();
      msgadd(0xf0);
      while(Mf_toberead >= lookfor) msgadd(c=egetc());
      if (c == 0xf7 || Mf_nomerge == 0)
        sysex();
      else
        sysexcontinue = 1;
      break;
     case 0xf7:
      lookfor = Mf_toberead - readvarinum();
      if (! sysexcontinue) msginit();
      while (Mf_toberead >= lookfor)  msgadd(c=egetc());
      if (! sysexcontinue) {
        if (Mf_arbitrary) (*Mf_arbitrary)(msgleng(), msg());
      } else if (c == 0xf7) {
        sysex();
        sysexcontinue = 0;
      }
      break;
     default:
      badbyte(c);
      break;
    }
  }
  if ( Mf_endtrack ) (*Mf_endtrack)();
  return(1);
}

static void badbyte(int c) {

  char buff[32];

  (void) sprintf(buff, "unexpected byte: 0x%02x", c);
  mferror(buff);
}

static int metaevent(int type) {

  int leng = msgleng();
  char *m = msg();

  switch (type) {
  case 0x00:
    if (Mf_seqnum) (*Mf_seqnum)(to16bit(m[0], m[1]));
    break;
  case 0x01:  /* Text event */
  case 0x02:  /* Copyright notice */
  case 0x03:  /* Sequence/Track name */
  case 0x04:  /* Instrument name */
  case 0x05:  /* Lyric */
  case 0x06:  /* Marker */
  case 0x07:  /* Cue point */
  case 0x08:
  case 0x09:
  case 0x0a:
  case 0x0b:
  case 0x0c:
  case 0x0d:
  case 0x0e:
  case 0x0f:
    if (Mf_text) (*Mf_text)(type, leng, m);
    break;
  case 0x2f:
    if (Mf_eot) (*Mf_eot)();
    break;
  case 0x51:
    if (Mf_tempo) (*Mf_tempo)(to32bit(0, m[0], m[1], m[2]));
    break;
  case 0x54:
    if (Mf_smpte)
      (*Mf_smpte)(m[0], m[1], m[2], m[3], m[4]);
    break;
  case 0x58:
    if (Mf_timesig)
      (*Mf_timesig)(m[0], m[1], m[2], m[3]);
    break;
  case 0x59:
    if (Mf_keysig) (*Mf_keysig)(m[0], m[1]);
    break;
  case 0x7f:
    if (Mf_sqspecific) (*Mf_sqspecific)(leng, m);
    break;
  default:
    if (Mf_metamisc) (*Mf_metamisc)(type, leng, m);
  }
}

static int sysex() {

  if (Mf_sysex) (*Mf_sysex)(msgleng(), msg());
}

static int chanmessage(int status, int c1, int c2) {

  int chan = status & 0xf;

  switch(status & 0xf0) {
   case 0x80: if (Mf_off) (*Mf_off)(chan, c1, c2); break;
   case 0x90: if (Mf_on) (*Mf_on)(chan, c1, c2); break;
   case 0xa0: if (Mf_pressure) (*Mf_pressure)(chan, c1, c2); break;
   case 0xb0: if (Mf_parameter) (*Mf_parameter)(chan, c1, c2); break;
   case 0xe0: if (Mf_pitchbend) (*Mf_pitchbend)(chan, c1, c2); break;
   case 0xc0: if (Mf_program) (*Mf_program)(chan, c1); break;
   case 0xd0: if (Mf_chanpressure) (*Mf_chanpressure)(chan, c1); break;
  }
}

static long readvarinum() {

  long value;
  int c;

  c = egetc();
  value = c;
  if (c & 0x80) {
    value &= 0x7f;
    do {
      c = egetc();
      value = (value << 7) + (c & 0x7f);
    } while (c & 0x80);
  }
  return (value);
}

static long to32bit(int c1, int c2, int c3, int c4) {

  long value = 0L;

  value = (c1 & 0xff);
  value = (value<<8) + (c2 & 0xff);
  value = (value<<8) + (c3 & 0xff);
  value = (value<<8) + (c4 & 0xff);
  return (value);
}

static int to16bit(int c1, int c2) {

  return ((c1 & 0xff ) << 8) + (c2 & 0xff);
}

static long read32bit() {

  int c1, c2, c3, c4;

  c1 = egetc();
  c2 = egetc();
  c3 = egetc();
  c4 = egetc();
  return to32bit(c1, c2, c3, c4);
}

static int read16bit() {

  int c1, c2;
  c1 = egetc();
  c2 = egetc();
  return to16bit(c1, c2);
}

void mferror(char *s) {

  if (Mf_error) (*Mf_error)(s);
  exit(1);
}

static void msginit() {

  Msgindex = 0;
}

static char * msg() {

  return(Msgbuff);
}

static int msgleng() {

  return(Msgindex);
}

static void msgadd(int c) {

  if (Msgindex >= Msgsize) biggermsg();
  Msgbuff[Msgindex++] = c;
}

static void biggermsg() {

  char *newmess;
  char *oldmess = Msgbuff;
  int oldleng = Msgsize;

  Msgsize += MSGINCREMENT;
  newmess = (char *) malloc((unsigned)(sizeof(char)*Msgsize));
  if (newmess == NULL) mferror("malloc error!");
  if (oldmess != NULL) {
    register char *p = newmess;
    register char *q = oldmess;
    register char *endq = &oldmess[oldleng];
    for( ; q!=endq ; p++, q++ ) *p = *q;
    free(oldmess);
  }
  Msgbuff = newmess;
}

void mfwrite(int format, int ntracks, int division, FILE *fp) {

  int i;
  void mf_w_track_chunk(), mf_w_header_chunk();

  if (Mf_putc == NULLFUNC)
    mferror("mfmf_write() called without setting Mf_putc");
  if (Mf_wtrack == NULLFUNC)
    mferror("mfmf_write() called without setting Mf_mf_writetrack");
  mf_w_header_chunk(format, ntracks, division);
  if (format == 1 && ( Mf_wtempotrack )) {
    mf_w_track_chunk(-1, fp, Mf_wtempotrack);
    ntracks--;
  }
  for(i = 0; i < ntracks; i++)
    mf_w_track_chunk(i, fp, Mf_wtrack);
}

void mf_w_track_chunk(which_track, fp, wtrack)
int which_track;
FILE *fp;
int (*wtrack)();
{
  unsigned long trkhdr, trklength;
  long offset, place_marker;
  void write16bit(), write32bit();

  trkhdr = MTrk;
  trklength = 0;

  offset = ftell(fp);
  write32bit(trkhdr);
  write32bit(trklength);

  Mf_numbyteswritten = 0L;
  laststat = 0;
  (*wtrack)(which_track);

  if (laststat != meta_event || lastmeta != end_of_track) {
    eputc(0);
    eputc(meta_event);
    eputc(end_of_track);
    eputc(0);
  }

  laststat = 0;
  place_marker = ftell(fp);
  if (fseek(fp, offset, 0) < 0)
    mferror("error seeking during final stage of write");
  trklength = Mf_numbyteswritten;
  write32bit(trkhdr);
  write32bit(trklength);
  fseek(fp, place_marker, 0);
}

void mf_w_header_chunk(int format, int ntracks, int division) {

  unsigned long ident, length;
  void write16bit(), write32bit();

  ident = MThd;
  length = 6;
  write32bit(ident);
  write32bit(length);
  write16bit(format);
  write16bit(ntracks);
  write16bit(division);
}

int mf_w_midi_event(
  unsigned long delta_time,
  unsigned int type,
  unsigned int chan,
  unsigned char *data,
  unsigned long size) {

  int i;
  unsigned char c;

  WriteVarLen(delta_time);
  c = type | chan;

  if(chan > 15)
    perror("error: MIDI channel greater than 16\n");
  if (!Mf_RunStat || laststat != c)
    eputc(c);
  laststat = c;
  for(i = 0; i < size; i++)
  eputc(data[i]);

  return(size);
}

int mf_w_meta_event(
  unsigned long delta_time,
  unsigned int type,
  unsigned char *data,
  unsigned long size) {

  int i;

  WriteVarLen(delta_time);
  eputc(meta_event);
  laststat = meta_event;
  eputc(type);
  lastmeta = type;
  WriteVarLen(size);
  for(i = 0; i < size; i++) {
    if (eputc(data[i]) != data[i]) return(-1);
  }
  return(size);
}

int mf_w_sysex_event(
  unsigned long delta_time,
  unsigned char *data,
  unsigned long size) {

  int i;

  WriteVarLen(delta_time);
  eputc(*data);
  laststat = 0;
  WriteVarLen(size-1);
  for(i = 1; i < size; i++) {
    if(eputc(data[i]) != data[i]) return(-1);
  }
  return(size);
}

void mf_w_tempo(unsigned long delta_time, unsigned long tempo) {

  WriteVarLen(delta_time);
  eputc(meta_event);
  laststat = meta_event;
  eputc(set_tempo);
  eputc(3);
  eputc((unsigned)(0xff & (tempo >> 16)));
  eputc((unsigned)(0xff & (tempo >> 8)));
  eputc((unsigned)(0xff & tempo));
}

unsigned long mf_sec2ticks(float secs, int division, unsigned int tempo) {

   return (long)(((secs * 1000.0) / 4.0 * division) / tempo);
}

void WriteVarLen(unsigned long value) {

  unsigned long buffer;

  buffer = value & 0x7f;
  while((value >>= 7) > 0) {
    buffer <<= 8;
    buffer |= 0x80;
    buffer += (value & 0x7f);
  }
  while(1) {
    eputc((unsigned)(buffer & 0xff));
    if (buffer & 0x80)
      buffer >>= 8;
    else
      return;
  }
}

float mf_ticks2sec(int ticks, unsigned int division, unsigned long tempo) {

  float smpte_format, smpte_resolution;

  if (division > 0) {
    return((float)(((float) (ticks)*(float)(tempo))/
    ((float)(division)*1000000.0)));
  } else {
     smpte_format = upperbyte(division);
     smpte_resolution = lowerbyte(division);
     return(float)((float) ticks/(smpte_format*smpte_resolution*1000000.0));
  }
}

void write32bit(unsigned long data) {

  eputc((unsigned)((data >> 24) & 0xff));
  eputc((unsigned)((data >> 16) & 0xff));
  eputc((unsigned)((data >> 8 ) & 0xff));
  eputc((unsigned)(data & 0xff));
}

void write16bit(int data) {

  eputc((unsigned)((data & 0xff00) >> 8));
  eputc((unsigned)(data & 0xff));
}

int eputc(unsigned char c) {

  int return_val;

  if ((Mf_putc) == NULLFUNC) {
    mferror("Mf_putc undefined");
    return(-1);
  }

  return_val = (*Mf_putc)(c);

  if (return_val == EOF) mferror("error writing");
  Mf_numbyteswritten++;
  return(return_val);
}

char *mknote(int pitch) {

  static char * Notes [] =
    {"c", "c#", "d", "d#", "e", "f", "f#", "g", "g#", "a", "a#", "b"};
  static char buf[5];
  if ( notes )
    sprintf (buf, "%s%d", Notes[pitch % 12], pitch/12);
  else
    sprintf (buf, "%d", pitch);
  return buf;
}

void myheader(int format, int ntrks, int division) {

  if (division & 0x8000) {
    times = 0;
    printf("MFile %d %d %d %d\n",
      format, ntrks, -((-(division>>8))&0xff), division&0xff);
  } else {
    printf("MFile %d %d %d\n", format, ntrks, division);
  }
  if (format > 2) {
    fprintf(stderr, "Can't deal with format %d files\n", format);
    exit (1);
  }
  Beat = Clicks = division;
  TrksToDo = ntrks;
}

void mytrstart() {

  printf("MTrk\n");
  TrkNr ++;
}

void mytrend() {

  printf("TrkEnd\n");
  --TrksToDo;
}

void mynon(int chan, int pitch, int vol) {

  prtime();
  printf(Onmsg, chan+1, mknote(pitch), vol);
}

void mynoff(int chan, int pitch, int vol) {

  prtime();
  printf(Offmsg, chan+1, mknote(pitch), vol);
}

void mypressure(int chan, int pitch, int press) {

  prtime();
  printf(PoPrmsg, chan+1, mknote(pitch), press);
}

void myparameter(int chan, int control, int value) {

  prtime();
  printf(Parmsg, chan+1, control, value);
}

void mypitchbend(int chan, int lsb, int msb) {

  prtime();
  printf(Pbmsg, chan+1, 128*msb+lsb);
}

void myprogram(int chan, int program) {

  prtime();
  printf(PrChmsg, chan+1, program);
}

void mychanpressure(int chan, int press) {

  prtime();
  printf(ChPrmsg, chan+1, press);
}

void mysysex(int leng, char *mess) {

  prtime();
  printf("SysEx");
  prhex (mess, leng);
}

void mymmisc(int type, int leng, char *mess) {

  prtime();
  printf("Meta 0x%02x", type);
  prhex(mess, leng);
}

void mymspecial(int leng, char *mess) {

  prtime();
  printf("SeqSpec");
  prhex(mess, leng);
}

void mymtext(int type, int leng, char *mess) {

  static char *ttype[] = {
    NULL,
    "Text", "Copyright", "TrkName", "InstrName", "Lyric", "Marker", "Cue", "Unrec"
  };
  int unrecognized = (sizeof(ttype)/sizeof(char *)) - 1;
  prtime();
  if (type < 1 || type > unrecognized)
    printf("Meta 0x%02x ", type);
  else if (type == 3 && TrkNr == 1)
    printf("Meta SeqName ");
  else
    printf("Meta %s ", ttype[type]);
  prtext (mess, leng);
}

void mymseq(int num) {

  prtime();
  printf("SeqNr %d\n", num);
}

void mymeot() {

  prtime();
  printf("Meta TrkEnd\n");
}

void mykeysig(int sf, int mi) {

  prtime();
  printf("KeySig %d %s\n", (sf > 127 ? sf-256 : sf), (mi ? "minor" : "major"));
}

void mytempo(long tempo) {

  prtime();
  printf("Tempo %ld\n", tempo);
}

void mytimesig(int nn, int dd, int cc, int bb) {

  int denom = 1;

  while (dd-- > 0) denom *= 2;
  prtime();
  printf("TimeSig %d/%d %d %d\n", nn, denom, cc, bb);
  M0 += (Mf_currtime-T0)/(Beat*Measure);
  T0 = Mf_currtime;
  Measure = nn;
  Beat = 4 * Clicks / denom;
}

void mysmpte(int hr, int mn, int se, int fr, int ff) {

  prtime();
  printf("SMPTE %d %d %d %d %d\n", hr, mn, se, fr, ff);
}

void myarbitrary(int leng, char *mess) {

  prtime();
  printf("Arb", leng);
  prhex(mess, leng);
}

void prtime() {
    if (times) 
      {
	if (incs)
	  {
	    long m = (Mf_currtime-old_Mf_currtime)/Beat;
	    if (verbose)
	      printf("%03ld:%02ld:%03ld ",
		     m/Measure+M0,m%Measure,(Mf_currtime-old_Mf_currtime)%Beat);
	    else
	      printf("%ld:%ld:%ld ",
		     m/Measure+M0,m%Measure,(Mf_currtime-old_Mf_currtime)%Beat);
	  }
	else
	  {
	    long m = (Mf_currtime-T0)/Beat;
	    if (verbose)
	      printf("%03ld:%02ld:%03ld ",
		     m/Measure+M0,m%Measure,(Mf_currtime-T0)%Beat);
	    else
	      printf("%ld:%ld:%ld ",
		     m/Measure+M0,m%Measure,(Mf_currtime-T0)%Beat);
	  }
      } 
    else 
      {
	if (incs)
	  {
	    if (verbose)
	      printf("%-10ld ", Mf_currtime - old_Mf_currtime);
	    else
	      printf("%ld ", Mf_currtime - old_Mf_currtime);
	  }
	else
	  {
	    if (verbose)
	      printf("%-10ld ", Mf_currtime);
	    else
	      printf("%ld ", Mf_currtime);
	  }
      }
}

void prtext(unsigned char *p, int leng) {

  int n, c;
  int pos = 25;

  printf("\"");
  for ( n=0; n<leng; n++ ) {
    c = *p++;
    if (fold && pos >= fold) {
      printf ("\\\n\t");
      pos = 13;  /* tab + \xab + \ */
      if (c == ' ' || c == '\t') {
        putchar ('\\');
        ++pos;
      }
    }
    switch (c) {
     case '\\':
     case '"':
      printf ("\\%c", c);
      pos += 2;
      break;
     case '\r':
      printf ("\\r");
      pos += 2;
      break;
     case '\n':
      printf ("\\n");
      pos += 2;
      break;
     case '\0':
      printf ("\\0");
      pos += 2;
      break;
     default:
      if (isprint(c)) {
        putchar(c);
        ++pos;
      } else {
        printf("\\x%02x" , c);
        pos += 4;
      }
    }
  }
  printf("\"\n");
}

void prhex(unsigned char *p, int leng) {

  int n;
  int pos = 25;

  for(n = 0; n < leng; n++, p++) {
    if (fold && pos >= fold) {
      printf ("\\\n\t%02x", *p);
      pos = 14;
    } else {
      printf(" %02x", *p);
      pos += 3;
    }
  }
  printf("\n");
}

myerror(char *s) {

  if (TrksToDo <= 0)
    fprintf(stderr, "Error: Garbage at end\n", s);
  else
    fprintf(stderr, "Error: %s\n", s);
}

void initfuncs() {

  Mf_error = myerror;
  Mf_header =  myheader;
  Mf_starttrack =  mytrstart;
  Mf_endtrack =  mytrend;
  Mf_on =  mynon;
  Mf_off =  mynoff;
  Mf_pressure =  mypressure;
  Mf_parameter =  myparameter;
  Mf_pitchbend =  mypitchbend;
  Mf_program =  myprogram;
  Mf_chanpressure =  mychanpressure;
  Mf_sysex =  mysysex;
  Mf_metamisc =  mymmisc;
  Mf_seqnum =  mymseq;
  Mf_eot =  mymeot;
  Mf_timesig =  mytimesig;
  Mf_smpte =  mysmpte;
  Mf_tempo =  mytempo;
  Mf_keysig =  mykeysig;
  Mf_sqspecific =  mymspecial;
  Mf_text =  mymtext;
  Mf_arbitrary =  myarbitrary;
}

void prs_error(char *s) {

  int c;
  int count;
  int ln = (eol_seen? lineno-1 : lineno);
  fprintf(stderr, "%d: %s\n", ln, s);
  if (yyleng > 0 && *yytext != '\n')
    fprintf(stderr, "*** %*s ***\n", yyleng, yytext);
  count = 0;
  while (count < 100 &&
     (c=yylex()) != EOL && c != EOF) count++/* skip rest of line */;
  if (c == EOF) exit(1);
  if (err_cont)
    longjmp(erjump, 1);
}

void syntax() {

  prs_error("Syntax error");
}

void translate() {

  if (yylex() == MTHD) {
    Format = getint("MFile format");
    Ntrks = getint("MFile #tracks");
    Clicks = getint("MFile Clicks");
    if (Clicks < 0)
      Clicks = (Clicks&0xff)<<8|getint("MFile SMPTE division");
    checkeol();
    mfwrite(Format, Ntrks, Clicks, F);
  } else {
    fprintf (stderr, "Missing MFile - can't continue\n");
    exit(1);
  }
}

static int mywritetrack() {

  int opcode, c;
  long currtime = 0;
  long newtime, delta;
  int i, k;

  while ((opcode = yylex()) == EOL) ;
  if (opcode != MTRK) prs_error("Missing MTrk");
  checkeol();
  while(1) {
    err_cont = 1;
    setjmp (erjump);
    switch(yylex()) {
     case MTRK:
      prs_error("Unexpected MTrk");
     case EOF:
      err_cont = 0;
      error("Unexpected EOF");
      return -1;
     case TRKEND:
      err_cont = 0;
      checkeol();
      return 1;
     case INT:
      newtime = yyval;
      if ((opcode = yylex()) == '/') {
        if (yylex() != INT) prs_error("Illegal time value");
        newtime = (newtime - M0) * Measure + yyval;
        if (yylex() != '/' || yylex() != INT) prs_error("Illegal time value");
        newtime = T0 + newtime * Beat + yyval;
        opcode = yylex();
      }
      if (incs)
	delta = newtime;
      else
	delta = newtime - currtime;
      if (delta < 0) prs_error("Illegal time value, did you forget -i option ?");
      switch(opcode) {
       case ON:
       case OFF:
       case POPR:
        checkchan();
        checknote();
        checkval();
        mf_w_midi_event(delta, opcode, chan, data, 2L);
        break;
       case PAR:
        checkchan();
        checkcon();
        checkval();
        mf_w_midi_event(delta, opcode, chan, data, 2L);
        break;
       case PB:
        checkchan();
        splitval();
        mf_w_midi_event(delta, opcode, chan, data, 2L);
        break;
       case PRCH:
        checkchan();
        checkprog();
        mf_w_midi_event(delta, opcode, chan, data, 1L);
        break;
       case CHPR:
        checkchan();
        checkval();
        data[0] = data[1];
        mf_w_midi_event(delta, opcode, chan, data, 1L);
        break;
       case SYSEX:
       case ARB:
        gethex();
        mf_w_sysex_event(delta, buffer, (long)buflen);
        break;
       case TEMPO:
        if (yylex() != INT) syntax();
        mf_w_tempo (delta, yyval);
        break;
       case TIMESIG: {
          int nn, denom, cc, bb;
          if (yylex() != INT || yylex() != '/') syntax();
          nn = yyval;
          denom = getbyte("Denom");
          cc = getbyte("clocks per click");
          bb = getbyte("32nd notes per 24 clocks");
          for(i = 0, k = 1 ; k < denom; i++, k <<= 1);
          if (k != denom) error("Illegal TimeSig");
          data[0] = nn;
          data[1] = i;
          data[2] = cc;
          data[3] = bb;
          M0 += (newtime - T0) / (Beat * Measure);
          T0 = newtime;
          Measure = nn;
          Beat = 4 * Clicks / denom;
          mf_w_meta_event(delta, time_signature, data, 4L);
        }
        break;
       case SMPTE:
        for(i = 0; i < 5; i++) data[i] = getbyte("SMPTE");
        mf_w_meta_event(delta, smpte_offset, data, 5L);
        break;
       case KEYSIG:
        data[0] = i = getint("Keysig");
        if (i < -7 || i > 7)
          error("Key Sig must be between -7 and 7");
        if ((c=yylex()) != MINOR && c != MAJOR)
          syntax();
        data[1] = (c == MINOR);
        mf_w_meta_event(delta, key_signature, data, 2L);
        break;
       case SEQNR:
        get16val("SeqNr");
        mf_w_meta_event(delta, sequence_number, data, 2L);
        break;
       case META: {
          int type = yylex();
          switch(type) {
           case TRKEND: type = end_of_track; break;
           case TEXT:
           case COPYRIGHT:
           case SEQNAME:
           case INSTRNAME:
           case LYRIC:
           case MARKER:
           case CUE: type -= (META+1); break;
           case INT: type = yyval; break;
           default: prs_error("Illegal Meta type");
          }
          if (type == end_of_track)
            buflen = 0;
          else
            gethex();
          mf_w_meta_event(delta, type, buffer, (long)buflen);
          break;
        }
       case SEQSPEC:
        gethex();
        mf_w_meta_event(delta, sequencer_specific, buffer, (long)buflen);
        break;
       default:
        prs_error("Unknown input");
        break;
      }
      currtime = newtime;
     case EOL:
      break;
     default:
      prs_error("Unknown input");
      break;
    }
    checkeol();
  }
}

getbyte(char *mess) {

  char ermesg[100];

  getint(mess);
  if (yyval < 0 || yyval > 127) {
    sprintf(ermesg, "Wrong value (%ld) for %s", yyval, mess);
    error(ermesg);
    yyval = 0;
  }
  return yyval;
}

getint(char *mess) {

  char ermesg[100];
  if (yylex() != INT) {
    sprintf(ermesg, "Integer expected for %s", mess);
    error(ermesg);
    yyval = 0;
  }
  return yyval;
}

static void checkchan() {

  if (yylex() != CH || yylex() != INT) syntax();
  if (yyval < 1 || yyval > 16) error("Chan must be between 1 and 16");
  chan = yyval-1;
}

static void checknote() {

  int c;

  if (yylex() != NOTE || ((c=yylex()) != INT && c != NOTEVAL))
  syntax();
  if (c == NOTEVAL) {
    static int notes[] = {9, 11, 0, 2, 4, 5, 7};
    char *p = yytext;
    c = *p++;
    if (isupper(c)) c = tolower(c);
    yyval = notes[c-'a'];
    switch(*p) {
     case '#':
     case '+': yyval++; p++; break;
     case 'b':
     case 'B':
     case '-': yyval--; p++; break;
     }
     yyval += 12 * atoi(p);
  }
  if (yyval < 0 || yyval > 127)
    error("Note must be between 0 and 127");
  data[0] = yyval;
}

static void checkval() {

  if (yylex() != VAL || yylex() != INT) syntax();
  if (yyval < 0 || yyval > 127)
    error("Value must be between 0 and 127");
  data[1] = yyval;
}

static void splitval() {

  if (yylex() != VAL || yylex() != INT) syntax();
  if (yyval < 0 || yyval > 16383)
     error("Value must be between 0 and 16383");
  data[0] = yyval % 128;
  data[1] = yyval / 128;
}

static void get16val() {

  if (yylex() != VAL || yylex() != INT) syntax();
  if (yyval < 0 || yyval > 65535)
    error("Value must be between 0 and 65535");
  data[0] = (yyval >> 8) & 0xff;
  data[1] = yyval & 0xff;
}

static void checkcon() {

  if (yylex() != CON || yylex() != INT)
  syntax();
  if (yyval < 0 || yyval > 127)
    error("Controller must be between 0 and 127");
  data[0] = yyval;
}

static void checkprog() {

  if (yylex() != PROG || yylex() != INT) syntax();
  if (yyval < 0 || yyval > 127)
    error("Program number must be between 0 and 127");
  data[0] = yyval;
}

static void checkeol() {

  if (eol_seen) return;
  if (yylex() != EOL) {
    prs_error ("Garbage deleted");
    while (!eol_seen) yylex();
  }
}

static void gethex() {

  int c;

  buflen = 0;
  do_hex = 1;
  c = yylex();
  if (c == STRING) {
    int i = 0;
    if (yyleng - 1 > bufsiz) {
      bufsiz = yyleng - 1;
      if (buffer)
        buffer = realloc(buffer, bufsiz);
      else
        buffer = malloc (bufsiz);
      if (! buffer) error("Out of memory");
    }
    while(i < yyleng - 1) {
      c = yytext[i++];
rescan:
      if (c == '\\') {
        switch (c = yytext[i++]) {
         case '0': c = '\0'; break;
         case 'n': c = '\n'; break;
         case 'r': c = '\r'; break;
         case 't': c = '\t'; break;
         case 'x':
          if (sscanf (yytext+i, "%2x", &c) != 1)
            prs_error ("Illegal \\x in string");
          i += 2;
          break;
         case '\r':
         case '\n':
          while ((c = yytext[i++]) == ' ' || c == '\t' || c == '\r' || c == '\n')
            goto rescan;
        }
      }
      buffer[buflen++] = c;
    }
  } else if (c == INT) {
    do {
      if (buflen >= bufsiz) {
        bufsiz += 128;
        if (buffer)
          buffer = realloc(buffer, bufsiz);
        else
           buffer = malloc(bufsiz);
        if (! buffer) error ("Out of memory");
      }
      buffer[buflen++] = yyval;
      c = yylex();
    } while (c == INT);
    if (c != EOL) prs_error("Unknown hex input");
  } else {
    prs_error("String or hex input expected");
  }
}

long bankno (char *s, int n) {

  long res = 0;
  int c;
  while(n-- > 0) {
    c = (*s++);
    if (islower(c))
      c -= 'a';
    else if (isupper(c))
      c -= 'A';
    else
      c -= '1';
    res = res * 8 + c;
  }
  return res;
}


FILE *efopen(char *name, char *mode) {
if (dbg) fprintf(stderr, "efopen(%s, %s)\n", name, mode);

  FILE *f;
  if ((f = fopen(name, mode)) == NULL) {
    (void) fprintf(stderr, "Cannot open '%s', %s!\n", name, strerror(errno));
    exit(1);
  }
  return(f);
}

fileputc(int c) {

  return putc(c, F);
}

int filegetc() {

  return(getc(F));
}

/***
* Version: v0.0.6 20110727
* License: AGPLv3 - http://www.gnu.org/licenses/agpl.html
* Copyright: 2003-2011 Mark Constable (markc@renta.net)
***/
