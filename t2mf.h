/* $Id: t2mf.h,v 1.2 1991/11/03 21:50:50 piet Rel $ */
#include "midifile.h"
#include <stdlib.h>

#define MTHD	256
#define MTRK	257
#define TRKEND	258

#define ON	note_on
#define OFF	note_off
#define POPR	poly_aftertouch
#define PAR	control_change
#define PB	pitch_wheel
#define PRCH	program_chng
#define CHPR	channel_aftertouch
#define SYSEX	system_exclusive

#define ARB	259
#define MINOR	260
#define MAJOR	261
	
#define CH	262
#define NOTE	263
#define VAL	264
#define CON	265
#define PROG	266

#define INT	267
#define STRING	268
#define STRESC	269
#define ERR	270
#define NOTEVAL 271
#define EOL	272

#define META	273
#define SEQSPEC	(META+1+sequencer_specific)
#define TEXT	(META+1+text_event)
#define COPYRIGHT	(META+1+copyright_notice)
#define SEQNAME	(META+1+sequence_name)
#define INSTRNAME	(META+1+instrument_name)
#define LYRIC	(META+1+lyric)
#define MARKER	(META+1+marker)
#define CUE	(META+1+cue_point)
#define SEQNR	(META+1+sequence_number)
#define KEYSIG	(META+1+key_signature)
#define TEMPO	(META+1+set_tempo)
#define TIMESIG	(META+1+time_signature)
#define SMPTE	(META+1+smpte_offset)
