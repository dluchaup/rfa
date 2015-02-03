##############################################################################
# File:    Makefile
# Author:  Daniel Luchaup
# Date:    23 June 2014
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
LTROOT = ../../lib/
endif

SRC=src

ifeq ($(BIN), )
BIN=bin
endif


CFLAGS = -g $(MAYBESTATIC) ${SVN_INFO} -frecord-gcc-switches -Wextra -Wall $(GPROF) -I${LTROOT}/include/  -I./include/ -I/home/dl/bin/lib/boost_1_51_0 #-DVERBOSE_DMP -DVERBOSE_TIME #--save-temps
CC   = g++ -g ${CFLAGS} -O3
LD   = g++ $(MAYBESTATIC) $(GPROF)

###############################################################################
${BIN}/%.o: ${SRC}/%.cc
	$(CC) -o $@ -c $<

${BIN}/%.o: ${BIN}/%.cc
	$(CC) -o $@ -c $<

###############################################################################
ifeq ($(LIBRANK), )
LIBRANK = ${LTROOT}/bin/rank.a
endif
###############################################################################
LIBS      = -L   ${LIBRANK} $(patsubst %, -l%, $(LIBRARIES))

###############################################################################
EXGSRCS= mdfa.cc mm.cc dominators.cc
EXGOBJS0=${EXGSRCS:.cc=.o}
EXGOBJS=$(addprefix ${BIN}/, ${EXGOBJS0})

###############################################################################
ALLSRCS= ${EXGSRCS}
ALLOBJS0=${ALLSRCS:.cc=.o}
ALLOBJS=$(addprefix ${BIN}/, ${ALLOBJS0})
###############################################################################
%.o: %.cc
	$(CC) -o $@ -c $<

.PHONY:	all etags tags depend clean showtrex showrank new newtest mod

EXECUTABLES= ${BIN}/mdfa

all:	 ${BIN} ${MAKEFILENAME} ${EXECUTABLES} ${LIBRANK}

###############################################################################
${BIN}:
	mkdir ${BIN}

${BIN}/mdfa: $(EXGOBJS)  ${LIBRANK}
	$(LD) -o $@  $^ $(LIBS)
###############################################################################

depend:
	makedepend -- $(CFLAGS) -- -Y $(ALLSRCS)

clean:
		@rm -f 	${EXECUTABLES} ${ALLOBJS}

new:
	make -f ${MAKEFILENAME} clean; make -f ${MAKEFILENAME} -j10

###############################################################################
# debugging stuff:
###############################################################################
newtest:
	make -f ${MAKEFILENAME} clean; make -f ${MAKEFILENAME} -j10; ./run_tests.sh

