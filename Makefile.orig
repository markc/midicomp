# $Id: makefile.unx,v 1.3 1991/11/15 19:31:00 piet Rel $

CFLAGS = -O
# If you have an old version of flex that defines yyleng
# as a macro rather than a variable, uncomment the following line:
# CFLAGS = -O -DNO_YYLENG_VAR

SRCS = midicomp.c t2mf.fl t2mflex.c yyread.c

EXECS = midicomp

all: $(EXECS)

midicomp: midicomp.h midicomp.o t2mf.h t2mflex.o
	$(CC) midicomp.o t2mflex.o -o midicomp

t2mflex.c: t2mf.fl
	flex -i -s -Ce t2mf.fl
	mv lex.yy.c t2mflex.c

t2mflex.o: t2mflex.c t2mf.h

clean:
	rm -f *.o midicomp

install:
	strip midicomp ;\
	cp midicomp /usr/bin ;\
	mv midicomp ../midicomp.exe

dist:
	tar czf ../midicomp-$$(cat VERSION).tar.gz \
		../midicomp-$$(cat VERSION) \
		--exclude=*.*html \
		--exclude=*.tar.gz \
		--exclude=*.exe \
		--exclude=*.o
