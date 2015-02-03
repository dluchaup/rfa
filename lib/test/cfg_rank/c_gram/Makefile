##############################################################################
# File:    Makefile
# Author:  Daniel Luchaup
# Date:    4 June 2014
# Copyright 2014 Daniel Luchaup luchaup@cs.wisc.edu
#
# This file contains unpublished confidential proprietary work of
# Daniel Luchaup, Department of Computer Sciences, University of
# Wisconsin--Madison.  No use of any sort, including execution, modification,
# copying, storage, distribution, or reverse engineering is permitted without
# the express written consent (for each kind of use) of Daniel Luchaup.
#
##############################################################################

MAKEFILENAME=$(lastword $(MAKEFILE_LIST))
EXGMAKEFILENAME=$(word 1, $(MAKEFILE_LIST))

LIBRARIES =  pthread gmpxx gmp

YACC=bison

###############################################################################
GRAM=c-gram
EXG=exG
ifeq ($(LTROOT), )
LTROOT = ../../../../lib
endif
ifeq ($(LIBRANK), )
LIBRANK = ${LTROOT}/bin/rank.a
#LIBRANK = ${LTROOT}/bin-O3-verbose/rank.a
endif

CFG2RANKER=../../../../tools/util/cfg2ranker.pl

SRC=src

ifeq ($(BIN), )
BIN=bin
endif

GPROF=#-pg
#MAYBESTATIC = -static -static-libgcc

CFLAGS = -g $(MAYBESTATIC) ${SVN_INFO} -frecord-gcc-switches -Wextra -Wall $(GPROF) -I${LTROOT}/include/ -I${BIN} -I${SRC} #-DVERBOSE_DMP -DVERBOSE_TIME #--save-temps
CC   = g++ ${CFLAGS} -O3 # -D_GLIBCXX_DEBUG
LD   = g++ $(MAYBESTATIC) $(GPROF)

###############################################################################

LIBS      = -L  $(patsubst %, -l%, $(LIBRARIES)) ${LIBRANK}

YACCFILE=${BIN}/${EXG}.y
LEXFILE=${BIN}/${EXG}.l
DEFSFILE=${BIN}/${EXG}_defs.h

###############################################################################
EXGSRCS= ${EXG}_parse.cc ${EXG}_main.cc ${EXG}_lex.cc
EXGOBJS=$(patsubst %, ${BIN}/%, ${EXGSRCS:.cc=.o} )

###############################################################################
###############################################################################
ALLSRCS= ${EXGSRCS}
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
${BIN}/%_parse.cc: ${BIN}/%.y
	#$(YACC) -p${EXG}_ -v -d  -o $@ $<
	$(YACC) -p$(subst ${BIN}/,, $(patsubst %.y, %,$<))_ -v -d  -o $@ $<
${BIN}/%_lex.cc: ${BIN}/%.l
	#lex -P${EXG}_ -o$@ $<
	lex -P$(subst ${BIN}/,, $(patsubst %.l,%,$<))_ -o$@ $<
${BIN}/%_parse.hh: ${BIN}/%_parse.cc # (1)
	#nothing

.PHONY:	all etags tags depend clean showtrex showrank new newtest mod

EXECUTABLES= ${BIN}/${EXG}
all:	${BIN} ${MAKEFILENAME} ${EXGMAKEFILENAME} ${EXECUTABLES} ${LIBRANK}

tests: all
	scripts/cfg_c_gram_tests.sh

${BIN}:
	mkdir ${BIN}

###############################################################################
${YACCFILE}: $(filter-out $(wildcard ${BIN}), ${BIN}) ${GRAM}
	@cd $$(dirname $@); ../${CFG2RANKER} ../${GRAM} 2>&1 2> LOG.make > LOG.make

${LEXFILE}: ${YACCFILE}  ${GRAM} #not really ${YACCFILE}, but ok ...
${DEFSFILE}: ${YACCFILE} ${GRAM} #not really ${YACCFILE}, but ok ...

${BIN}/${EXG}_main.o: ${DEFSFILE}

${BIN}/${EXG}: $(EXGOBJS) ${LIBRANK}
	$(LD) -lpthread -o $@  $^ $(LIBS)
###############################################################################

etags:
	svn info *.[chlyCH]* 2> /dev/null| awk '/^Path:/{print $$2}' | xargs etags

tags:
		find . -name "*.[ch]*" -print | etags -

depend:
	makedepend -- $(CFLAGS) -- -Y $(ALLSRCS)

${BIN}/${EXG}_lex.cc:	${LEXFILE} ${YACCFILE:.y=_parse.hh} ${DEFSFILE}
${BIN}/${EXG}_parse.cc:	${YACCFILE}  ${DEFSFILE}


clean:
	@rm -rf ${BIN} tmp

cleanall: clean

#@rm -f 	${EXECUTABLES} ${EXGOBJS} ${YACCFILE:.y=_parse.cc} ${YACCFILE:.y=_parse.hh} ${YACCFILE:.y=_parse.output} ${LEXFILE:.l=_lex.cc}

new:
	make -f ${EXGMAKEFILENAME} clean; make -f ${EXGMAKEFILENAME} -j10

###############################################################################
# debugging stuff:
###############################################################################
newtest:
	make -f ${MAKEFILENAME} clean; make -f ${MAKEFILENAME} -j10; ./run_tests.sh

