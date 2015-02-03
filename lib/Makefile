##############################################################################
# File:    Makefile
# Author:  Daniel Luchaup
# Date:    1 December 2013
# Copyright 2007-2014 Daniel Luchaup luchaup@cs.wisc.edu
#
# This file contains unpublished confidential proprietary work of
# Daniel Luchaup, Department of Computer Sciences, University of
# Wisconsin--Madison.  No use of any sort, including execution, modification,
# copying, storage, distribution, or reverse engineering is permitted without
# the express written consent (for each kind of use) of Daniel Luchaup.
#
# History: based on legacy code of Randy Smith
##############################################################################

GIT_COUNT=$(shell git rev-list --count HEAD)
GIT_CHANGED=$(shell git status| grep -e '#\s*modified:'|awk '{print $3}' | sort)
#GIT_INFO=-DGIT_COUNT=$(GIT_COUNT) -DGIT_CHANGED='"$(GIT_CHANGED)"'
MAKEFILENAME=$(lastword $(MAKEFILE_LIST))

YACC=bison

GPROF=#-pg
#MAYBESTATIC = -static -static-libgcc

SRC=src
ifeq ($(BIN), )
BIN=bin
endif
INCLUDE=include

CFLAGS = -g $(MAYBESTATIC) ${SVN_INFO} -frecord-gcc-switches -Wextra -Wall $(GPROF) ${EXTRAFLAGS} -I${INCLUDE} -I${BIN} #-DVERBOSE_DMP -DVERBOSE_TIME #--save-temps
CC   = g++ ${CFLAGS} -O3 # -D_GLIBCXX_DEBUG
LD   = g++ $(MAYBESTATIC) $(GPROF)

REXRANKSRCS=$(shell ls src/*.cc) ${BIN}/pcre_lex.cc ${BIN}/pcre_parse.cc

FYIREXPARSESRCS= ${BIN}/pcre_lex.cc ${BIN}/pcre_parse.cc \
	parse_utils.cc nodes.cc globals.cc errmsg.cc escape_sequences.cc


LIBSRCS=$(shell ls src/*.cc) ${BIN}/pcre_lex.cc ${BIN}/pcre_parse.cc
TEMPOBJS=${LIBSRCS:.cc=.o}
LIBOBJS=$(subst  src/, ${BIN}/, $(TEMPOBJS))

YACCFILES=pcre.y
LEXFILES=pcre.l
###############################################################################
${BIN}/%.o: ${SRC}/%.cc
	$(CC) -o $@ -c $<

${BIN}/%.o: ${BIN}/%.cc
	$(CC) -o $@ -c $<

# The following two rules assume that matching .y and .l files have the same
# basename, such as pcre.y and pcre.l
# That base name will be used as a prefix instead of yy.... For instance
# for pcre.y instead of yylval we'll use pcre_lval (see nodes.h for this
# particular case).
# This is done so we can use mutiple parsers inside a program.
${BIN}/%_parse.cc: ${SRC}/%.y
	$(YACC) -p$(subst ${SRC}/,, $(patsubst %.y, %,$<))_ -v -d  -o $@ $< #echo  $(patsubst %.y,%,$<)
${BIN}/%_lex.cc: ${SRC}/%.l
	lex -P$(subst ${SRC}/,, $(patsubst %.l,%,$<))_ -o$@ $<
${BIN}/%_parse.hh: ${BIN}/%_parse.cc # (1)
	#nothing

.PHONY:	all etags tags depend clean showtrex srcs new newtest mod

all:	${BIN} ${MAKEFILENAME} ${BIN}/rank.a

tests:
	@ make -f ${MAKEFILENAME} -j10 BIN=${BIN} && \
	  make -s -C ../tools/rex-intersect && make -s -C test

${BIN}:
	mkdir ${BIN}

verbose:
	make EXTRAFLAGS="-DVERBOSE_DMP -DVERBOSE_TIME" BIN=${BIN}

###############################################################################
${BIN}/rank.a:	$(LIBOBJS)
	ar rsc ${BIN}/rank.a $(LIBOBJS)
###############################################################################

etags:
	svn info *.[chlyCH]* 2> /dev/null| awk '/^Path:/{print $$2}' | xargs etags

tags:
		find . -name "*.[ch]*" -print | etags -

srcs:
	@echo ${LIBSRCS} pcre.l pcre.y

depend:
	makedepend -- $(CFLAGS) -- -Y $(LIBSRCS) &&\
	cat Makefile| awk '/^# DO NOT DELETE/{after=1;}{if (after==1){sub(/^src\//,"bin/");}; print}' > Makefile.tmp && mv Makefile.tmp Makefile

${BIN}/pcre_lex.cc:	${SRC}/pcre.l ${BIN}/pcre_parse.hh ${INCLUDE}/escape_sequences.h
${BIN}/pcre_parse.cc:	${SRC}/pcre.y ${INCLUDE}/nodes.h ${INCLUDE}/errmsg.h

clean:
	@rm -f ${BIN}/*

cleanall: clean
	@rmdir bin && make -C test cleanall \
	&& make -C ../tools/rex-intersect clean \
	&& make -C ../tools/build-rex-dfas clean


new:
	make -f ${MAKEFILENAME} clean; make -f ${MAKEFILENAME} -j10 BIN=${BIN}

###############################################################################
# debugging stuff:
###############################################################################

mod:
	@svn diff | grep -e '^Index' | awk '{print $$2}'
# DO NOT DELETE

bin/dbg.o: include/dbg.h
bin/dfa_tab_t.o: include/dfa_tab_t.h include/utils.h include/dbg.h
bin/dfa_tab_t.o: include/trex.h include/uidObject.h include/alphabet_byte.h
bin/dfa_tab_t.o: include/MPInt.h include/tnfa.h include/MyTime.h
bin/escape_sequences.o: include/escape_sequences.h include/errmsg.h
bin/ffa.o: include/ffa.h include/ranker.h include/trex.h include/uidObject.h
bin/ffa.o: include/alphabet_byte.h include/MPInt.h include/tnfa.h
bin/ffa.o: include/utils.h include/MyTime.h include/dfa_tab_t.h include/dbg.h
bin/ffa.o: include/escape_sequences.h
bin/globals.o: include/globals.h include/alphabet_byte.h
bin/gram.o: include/gram.h include/MPInt.h include/ranker.h include/trex.h
bin/gram.o: include/uidObject.h include/alphabet_byte.h include/tnfa.h
bin/gram.o: include/utils.h include/MyTime.h include/dfa_tab_t.h
bin/gram.o: include/dbg.h
bin/gram_tests.o: include/gram.h include/MPInt.h include/ranker.h
bin/gram_tests.o: include/trex.h include/uidObject.h include/alphabet_byte.h
bin/gram_tests.o: include/tnfa.h include/utils.h include/MyTime.h
bin/gram_tests.o: include/dfa_tab_t.h
bin/MPInt.o: include/MPInt.h
bin/nodes.o: include/nodes.h include/escape_sequences.h include/globals.h
bin/nodes.o: include/alphabet_byte.h include/trex.h include/uidObject.h
bin/nodes.o: include/MPInt.h include/zoptions.h include/MyTime.h
bin/nodes.o: include/parse_utils.h
bin/parse_utils.o: include/nodes.h include/parse_utils.h include/globals.h
bin/parse_utils.o: include/alphabet_byte.h
bin/ranker.o: include/zoptions.h include/ranker.h include/trex.h
bin/ranker.o: include/uidObject.h include/alphabet_byte.h include/MPInt.h
bin/ranker.o: include/tnfa.h include/utils.h include/MyTime.h
bin/ranker.o: include/dfa_tab_t.h include/escape_sequences.h include/dbg.h
bin/ranker.o: include/MemoryInfo.h
bin/tnfa.o: include/tnfa.h include/trex.h include/uidObject.h
bin/tnfa.o: include/alphabet_byte.h include/MPInt.h include/utils.h
bin/tnfa.o: include/MyTime.h include/escape_sequences.h include/ranker.h
bin/tnfa.o: include/dfa_tab_t.h include/dbg.h include/zoptions.h
bin/trex.o: include/trex.h include/uidObject.h include/alphabet_byte.h
bin/trex.o: include/MPInt.h include/tnfa.h include/utils.h include/MyTime.h
bin/trex_dbg.o: include/trex.h include/uidObject.h include/alphabet_byte.h
bin/trex_dbg.o: include/MPInt.h include/escape_sequences.h include/dbg.h
bin/trex_tests.o: include/trex.h include/uidObject.h include/alphabet_byte.h
bin/trex_tests.o: include/MPInt.h include/tnfa.h include/utils.h
bin/trex_tests.o: include/MyTime.h include/ranker.h include/dfa_tab_t.h
bin/trex_tests.o: include/ffa.h include/zoptions.h include/dbg.h
bin/trex_tests.o: include/MemoryInfo.h
bin/utils.o: include/utils.h
bin/zoptions.o: include/zoptions.h
bin/pcre_lex.o: bin/pcre_parse.hh include/escape_sequences.h include/nodes.h
bin/pcre_parse.o: include/errmsg.h include/nodes.h include/globals.h
bin/pcre_parse.o: include/alphabet_byte.h
