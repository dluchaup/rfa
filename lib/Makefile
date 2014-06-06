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

REXRANKSRCS=dfa_tab_t.cc dbg.cc \
	utils.cc trex.cc tnfa.cc ranker.cc \
	ffa.cc trex_dbg.cc MPInt.cc zoptions.cc

REXPARSESRCS=parse_utils.cc pcre_parse.cc pcre_lex.cc \
	nodes.cc globals.cc errmsg.cc \
	escape_sequences.cc

LIBSRCS0=${REXRANKSRCS} ${REXPARSESRCS} zoptions.cc
LIBSRCS=$(addprefix ${BIN}/, ${LIBSRCS0})

LIBOBJS=${LIBSRCS:.cc=.o}

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
	  make -s -C test

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
	makedepend -- $(CFLAGS) -- -Y $(LIBSRCS)

${BIN}/pcre_lex.cc:	${SRC}/pcre.l ${BIN}/pcre_parse.hh ${INCLUDE}/escape_sequences.h
${BIN}/pcre_parse.cc:	${SRC}/pcre.y ${INCLUDE}/nodes.h ${INCLUDE}/errmsg.h

clean:
	@rm -f ${BIN}/*

cleanall: clean
	@rmdir bin && make -C test cleanall


new:
	make -f ${MAKEFILENAME} clean; make -f ${MAKEFILENAME} -j10 BIN=${BIN}

###############################################################################
# debugging stuff:
###############################################################################

mod:
	@svn diff | grep -e '^Index' | awk '{print $$2}'


