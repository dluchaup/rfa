##############################################################################
# File:    Makefile
# Author:  Daniel Luchaup
# Copyright 2007-2015 Daniel Luchaup luchaup@cs.wisc.edu
#
# This file contains unpublished confidential proprietary work of
# Daniel Luchaup, Department of Computer Sciences, University of
# Wisconsin--Madison.  No use of any sort, including execution, modification,
# copying, storage, distribution, or reverse engineering is permitted without
# the express written consent (for each kind of use) of Daniel Luchaup.
#
##############################################################################

YACC=bison

###############################################################################
GRAM=GD-gram

ifeq ($(EXG), )
EXG=exG
endif

ifeq ($(LTROOT), )
LTROOT = ../../../lib
endif

ifeq ($(LIBRANK), )
LIBRANK = ${LTROOT}/bin/rank.a
endif

ifeq ($(CFG2RANKER), )
CFG2RANKER = ${LTROOT}/../tools/util/cfg2ranker.pl
endif

SRC=src

ifeq ($(BIN), )
BIN=bin
endif

CFLAGS = -O3 -g $(MAYBESTATIC) ${SVN_INFO} -frecord-gcc-switches -Wextra -Wall -I${LTROOT}/include/ -I${BIN} -I${SRC}
CC   = g++ ${CFLAGS}
LD   = g++

LIBS      = -L  -lpthread -lgmpxx -lgmp ${LIBRANK}

###############################################################################
YACCFILE=${EXG}.y
LEXFILE=${EXG}.l
###############################################################################
#EXGSRCS= main.cc ${EXG}_parse.cc ${EXG}_lex.cc
#EXGOBJS=$(patsubst %, ${BIN}/%, ${EXGSRCS:.cc=.o} )

EXGSRCS= main.cc
EXGOBJS=$(patsubst %, ${BIN}/%, ${EXGSRCS:.cc=.o} )  ${BIN}/${EXG}_lex.o ${BIN}/${EXG}_parse.o

.PHONY:	all clean new

EXECUTABLES= ${BIN}/${EXG}
all:	${BIN} ${EXECUTABLES} ${LIBRANK} #Makefile

${BIN}:
	mkdir ${BIN}

###############################################################################
${BIN}/main.o: ${SRC}/main.cc ${BIN}/${EXG}_defs.h
	$(CC) -o $@ -c $<

${BIN}/%.o: ${BIN}/%.cc
	$(CC) -o $@ -c $<

# The following two rules assume that matching .y and .l files have the same
# basename, such as pcre.y and pcre.l
# That base name will be used as a prefix instead of yy.... For instance
# for pcre.y instead of yylval we'll use pcre_lval (see nodes.h for this
# particular case).
# This is done so we can use mutiple parsers inside a program.
${BIN}/%_parse.cc: ${BIN}/%.y
	$(YACC) -p$(subst ${BIN}/,, $(patsubst %.y, %,$<))_ -v -d  -o $@ $<
${BIN}/%_lex.cc: ${BIN}/%.l
	lex -P$(subst ${BIN}/,, $(patsubst %.l,%,$<))_ -o$@ $<
${BIN}/%_parse.hh: ${BIN}/%_parse.cc # (1)
	#nothing

${BIN}/${EXG}: $(EXGOBJS)
	$(LD) -lpthread -o $@  $^ $(LIBS)
###############################################################################

${BIN}/${EXG}_lex.cc:	${BIN}/${LEXFILE} ${BIN}/${EXG}_parse.hh ${BIN}/${EXG}_defs.h
${BIN}/${EXG}_parse.cc:	${BIN}/${YACCFILE}  ${BIN}/${EXG}_defs.h



${BIN}/${LEXFILE}:	${BIN}/${EXG}_defs.h
${BIN}/${YACCFILE}:	${BIN}/${EXG}_defs.h

${BIN}/${EXG}_defs.h: ${SRC}/${GRAM} |${BIN}
	cd ${BIN} && ../${CFG2RANKER} ../${SRC}/${GRAM}


clean:
		@rm -f 	${EXECUTABLES} ${EXGOBJS} \
		       	${BIN}/${YACCFILE:.y=_parse.cc} \
			${BIN}/${YACCFILE:.y=_parse.hh} \
			${BIN}/${YACCFILE:.y=_parse.output} \
			${BIN}/${LEXFILE:.l=_lex.cc} \
			${BIN}/${YACCFILE} \
			${BIN}/${LEXFILE}  \
			${BIN}/${EXG}_defs.h

new:
	make clean; make -j10



ifeq ($(SLICE), )
SLICE=4000
endif

test: ${BIN}/${EXG}
	@echo Running tests for SLICE=${SLICE}
	${BIN}/${EXG} -zforced_len:${SLICE} | tee log.test-result-${SLICE}
