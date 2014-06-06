##############################################################################
# File:    Makefile
# Author:  Daniel Luchaup
# Date:    28 May 2014
# Copyright 2007-2014 Daniel Luchaup luchaup@cs.wisc.edu
#
# This file contains unpublished confidential proprietary work of
# Daniel Luchaup, Department of Computer Sciences, University of
# Wisconsin--Madison.  No use of any sort, including execution, modification,
# copying, storage, distribution, or reverse engineering is permitted without
# the express written consent (for each kind of use) of Daniel Luchaup.
#
##############################################################################

MAKEFILENAME=$(lastword $(MAKEFILE_LIST))

where=$(shell uname -a | awk '{print $$2}' )

LIBRARIES =  pthread gmpxx gmp

GPROF=#-pg
#MAYBESTATIC = -static -static-libgcc

###############################################################################
ifeq ($(LTROOT), )
LTROOT = ../..
endif
ifeq ($(LIBRANK), )
LIBRANK = ${LTROOT}/bin/rank.a
endif
###############################################################################

SRC=src

ifeq ($(BIN), )
BIN=bin
endif

CFLAGS = -g $(MAYBESTATIC) ${SVN_INFO} -frecord-gcc-switches -Wextra -Wall $(GPROF) -I${LTROOT}/include/  #-DVERBOSE_DMP -DVERBOSE_TIME #--save-temps
CC   = g++ -g ${CFLAGS} -O3
LD   = g++ $(MAYBESTATIC) $(GPROF)

###############################################################################
${BIN}/%.o: ${SRC}/%.cc
	$(CC) -o $@ -c $<

${BIN}/%.o: ${BIN}/%.cc
	$(CC) -o $@ -c $<

###############################################################################
LIBS      = -L  $(patsubst %, -l%, $(LIBRARIES)) ${LIBRANK}

###############################################################################
EXGSRCS= fa_rank_tests.cc
EXGOBJS0=${EXGSRCS:.cc=.o}
EXGOBJS=$(addprefix ${BIN}/, ${EXGOBJS0})

###############################################################################
ALLSRCS= ${EXGSRCS}
ALLOBJS0=${ALLSRCS:.cc=.o}
ALLOBJS=$(addprefix ${BIN}/, ${ALLOBJS0})
###############################################################################
.PHONY:	all etags tags depend clean showtrex showrank new newtest mod

EXECUTABLES= ${BIN}/fa_rank_tests

all:	 ${BIN} ${MAKEFILENAME} ${EXECUTABLES} ${LIBRANK}

tests: all
	@./scripts/fa_rank_tests.sh

###############################################################################
${BIN}:
	mkdir ${BIN}

${BIN}/fa_rank_tests: ${BIN}/fa_rank_tests.o ${LIBRANK} #$(EXGOBJS)
	$(LD) -lpthread -o $@  $^ $(LIBS)
###############################################################################

depend:
	makedepend -- $(CFLAGS) -- -Y $(ALLSRCS)

clean:
		@rm -rf 	${BIN} tmp
		#@rm -rf 	${EXECUTABLES} ${ALLOBJS} tmp

cleanall: clean

new:
	make -f ${MAKEFILENAME} clean; make -f ${MAKEFILENAME} -j10

###############################################################################
# debugging stuff:
###############################################################################
newtest:
	make -f ${MAKEFILENAME} clean; make -f ${MAKEFILENAME} -j10; ./run_tests.sh

