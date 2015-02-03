#!/bin/bash
SCRIPTDIR=$(dirname $(readlink -f $0) )
BASE=$(dirname $SCRIPTDIR)
#echo $BASE; exit 0
EXE=$BASE/bin/exG
DIR_TEST=$BASE/tmp/TST
DIR_GOLD=$BASE/GOLD/TST
#CHECKER=valgrind --leak-check=full
#MISC="$CHECKER"

if [ ! -d tmp ] ; then
    mkdir tmp
fi

if [ ! -d ${DIR_TEST} ] ; then
    mkdir ${DIR_TEST}
fi

if [ ! -d ${DIR_GOLD} ] ; then
    pushd $BASE
    tar xf GOLD.tar.bz2
    popd
fi

if [ -d ${DIR_TEST} ] ; then
    rm ${DIR_TEST}/* 2> /dev/null
    cd ${DIR_TEST}
    CMD="$EXE $MISC "
    #echo $CMD
    if $CMD > LOG ; then
	diff -r -w --ignore-matching-lines='Timer\|Command line:\|MemoryInfo:' ${DIR_GOLD} ${DIR_TEST} -xlog.diff.err 2>&1 | tee log.diff.err
	if [[ ! -s log.diff.err ]] ; then # i.e. if log.diff.err is not empty
	    echo "PASSED $(basename $0)"
	else
	    echo "DIFFERENT $(basename $0) FAILED"
	fi
    else
	echo "$EXE FAILED!"
    fi
else
    echo cannot find TST directory ${DIR_TEST}
fi
