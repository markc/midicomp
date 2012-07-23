/***
# midicomp

A MIDI Compiler - convert SMF MIDI files to and from plain text.
***/

#define MThd            0x4d546864L
#define MTrk            0x4d54726bL
#define MTHD            256
#define MTRK            257
#define TRKEND          258

#define ON              note_on
#define OFF             note_off
#define POPR            poly_aftertouch
#define PAR             control_change
#define PB              pitch_wheel
#define PRCH            program_chng
#define CHPR            channel_aftertouch
#define SYSEX           system_exclusive

#define ARB             259
#define MINOR           260
#define MAJOR           261

#define CH              262
#define NOTE            263
#define VAL             264
#define CON             265
#define PROG            266

#define INT             267
#define STRING          268
#define STRESC          269
#define ERR             270
#define NOTEVAL         271
#define EOL             272

#define META            273
#define SEQSPEC         (META+1+sequencer_specific)
#define TEXT            (META+1+text_event)
#define COPYRIGHT       (META+1+copyright_notice)
#define SEQNAME         (META+1+sequence_name)
#define INSTRNAME       (META+1+instrument_name)
#define LYRIC           (META+1+lyric)
#define MARKER          (META+1+marker)
#define CUE             (META+1+cue_point)
#define SEQNR           (META+1+sequence_number)
#define KEYSIG          (META+1+key_signature)
#define TEMPO           (META+1+set_tempo)
#define TIMESIG         (META+1+time_signature)
#define SMPTE           (META+1+smpte_offset)

#define note_off        0x80
#define note_on         0x90
#define poly_aftertouch 0xa0
#define control_change  0xb0
#define program_chng    0xc0
#define channel_aftertouch 0xd0
#define pitch_wheel     0xe0
#define system_exclusive 0xf0
#define delay_packet    (1111)

#define damper_pedal    0x40
#define portamento      0x41
#define sostenuto       0x42
#define soft_pedal      0x43
#define general_4       0x44
#define hold_2          0x45
#define general_5       0x50
#define general_6       0x51
#define general_7       0x52
#define general_8       0x53
#define tremolo_depth   0x5c
#define chorus_depth    0x5d
#define detune          0x5e
#define phaser_depth    0x5f

#define data_inc        0x60
#define data_dec        0x61

#define non_reg_lsb     0x62
#define non_reg_msb     0x63
#define reg_lsb         0x64
#define reg_msb         0x65

#define meta_event      0xFF
#define sequence_number 0x00
#define text_event      0x01
#define copyright_notice 0x02
#define sequence_name   0x03
#define instrument_name 0x04
#define lyric           0x05
#define marker          0x06
#define cue_point       0x07
#define channel_prefix  0x20
#define end_of_track    0x2f
#define set_tempo       0x51
#define smpte_offset    0x54
#define time_signature  0x58
#define key_signature   0x59
#define sequencer_specific 0x7f

#define Seq_Circuits    (0x01) /* Sequential Circuits Inc. */
#define Big_Briar       (0x02) /* Big Briar Inc.           */
#define Octave          (0x03) /* Octave/Plateau           */
#define Moog            (0x04) /* Moog Music               */
#define Passport        (0x05) /* Passport Designs         */
#define Lexicon         (0x06) /* Lexicon                  */
#define Tempi           (0x20) /* Bon Tempi                */
#define Siel            (0x21) /* S.I.E.L.                 */
#define Kawai           (0x41)
#define Roland          (0x42)
#define Korg            (0x42)
#define Yamaha          (0x43)

#define lowerbyte(x)    ((unsigned char)(x & 0xff))
#define upperbyte(x)    ((unsigned char)((x & 0xff00)>>8))
#define NULLFUNC        0
#define MSGINCREMENT    128

#ifdef NO_YYLENG_VAR
#define yyleng          yylength
#endif

static FILE *F;

static char *Msgbuff    = NULL;
static int Msgsize      = 0;
static int Msgindex     = 0;

static int TrkNr;
static int TrksToDo     = 1;
static int Measure, M0, Beat, Clicks;
static long T0;

static int fold         = 0;
static int notes        = 0;
static int times        = 0;
static int incs         = 0;
static char *Onmsg      = "On ch=%d n=%s v=%d\n";
static char *Offmsg     = "Off ch=%d n=%s v=%d\n";
static char *PoPrmsg    = "PoPr ch=%d n=%s v=%d\n";
static char *Parmsg     = "Par ch=%d c=%d v=%d\n";
static char *Pbmsg      = "Pb ch=%d v=%d\n";
static char *PrChmsg    = "PrCh ch=%d p=%d\n";
static char *ChPrmsg    = "ChPr ch=%d v=%d\n";
static jmp_buf erjump;
static int err_cont     = 0;
static int TrkNr;
static int Format, Ntrks;
static int Measure, M0, Beat, Clicks;
static long T0;
static char* buffer     = 0;
static int buflen,bufsiz= 0;

extern long yyval;
extern int yyleng;
extern int lineno;
extern char *yytext;
extern int do_hex;
extern int eol_seen;
extern FILE  *yyin;

static int  mywritetrack();
static void checkchan();
static void checknote();
static void checkval();
static void splitval();
static void get16val();
static void checkcon();
static void checkprog();
static void checkeol();
static void gethex();

int (*Mf_getc)()        = NULLFUNC;
int (*Mf_error)()       = NULLFUNC;
void (*Mf_header)()      = NULLFUNC;
void (*Mf_starttrack)()  = NULLFUNC;
void (*Mf_endtrack)()    = NULLFUNC;
void (*Mf_on)()          = NULLFUNC;
void (*Mf_off)()         = NULLFUNC;
void (*Mf_pressure)()    = NULLFUNC;
void (*Mf_parameter)()   = NULLFUNC;
void (*Mf_pitchbend)()   = NULLFUNC;
void (*Mf_program)()     = NULLFUNC;
void (*Mf_chanpressure)() = NULLFUNC;
void (*Mf_sysex)()       = NULLFUNC;
void (*Mf_arbitrary)()   = NULLFUNC;
void (*Mf_metamisc)()    = NULLFUNC;
void (*Mf_seqnum)()      = NULLFUNC;
void (*Mf_eot)()         = NULLFUNC;
void (*Mf_smpte)()       = NULLFUNC;
void (*Mf_tempo)()       = NULLFUNC;
void (*Mf_timesig)()     = NULLFUNC;
void (*Mf_keysig)()      = NULLFUNC;
void (*Mf_sqspecific)()  = NULLFUNC;
void (*Mf_text)()        = NULLFUNC;

static void badbyte();

static int readtrack();
static int metaevent();
static int sysex();
static int chanmessage();
static int msgleng();

static void msginit();
static void msgadd();
static void biggermsg();

float mf_ticks2sec();
unsigned long mf_sec2ticks(float,int,unsigned int);
void mfwrite();

int (*Mf_putc)()        = NULLFUNC;
int (*Mf_wtrack)()      = NULLFUNC;
int (*Mf_wtempotrack)() = NULLFUNC;

int Mf_nomerge          = 0;
long Mf_currtime        = 0L;
long old_Mf_currtime        = 0L;

static long Mf_toberead = 0L;
static long Mf_numbyteswritten = 0L;

static long readvarinum();
static long read32bit();
static long to32bit();
static int read16bit();
static int to16bit();
static char *msg();
static void readheader();

static int verbose_flag;
static int dbg=0;
char data[5];
int chan;
int Mf_RunStat = 0;
static int laststat;
static int lastmeta;
int verbose = 0;

int filegetc();
int fileputc();
void WriteVarLen();
int eputc(unsigned char);

void translate();
void initfuncs();
void mfread();
void mferror();
void error();

void mymseq();
void mymeot();
void mykeysig();
void mytempo();
void mytimesig();
void mysmpte();
void myarbitrary();
void prtime();
void prtext();
void prhex();
void initfuncs();
void prs_error();
void syntax();

/***
* Version: v0.0.6 20110727
* License: AGPLv3 - http://www.gnu.org/licenses/agpl.html
* Copyright: 2003-2011 Mark Constable (markc@renta.net)
***/
