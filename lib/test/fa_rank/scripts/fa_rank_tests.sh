#!/bin/bash
SCRIPTDIR=$(dirname $(readlink -f $0) )
BASE=$(dirname $SCRIPTDIR)
#echo $BASE; exit 0
EXE=$BASE/bin/fa_rank_tests
DIR_TEST=$BASE/tmp/FA_RANK_TST
DIR_GOLD=$BASE/GOLD/FA_RANK_TST
FILE_REX=$BASE/data/TEST_REXs
#CHECKER=valgrind --leak-check=full
#MISC="$CHECKER"

if [ ! -d tmp ] ; then
    mkdir tmp
fi

if [ ! -d ${DIR_TEST} ] ; then
    mkdir ${DIR_TEST}
fi

if [ ! -d ${DIR_GOLD} ] ; then
    pushd $BASE > /dev/null
    tar xf GOLD.tar.bz2
    popd > /dev/null
fi

if [ -d ${DIR_TEST} ] ; then
    rm ${DIR_TEST}/* 2> /dev/null
    cd ${DIR_TEST}
    #CMD="$EXE $MISC -zrexExprFile:${FILE_REX} -zforced_len:400 -zallow_ambiguous -zdo_dfa -zdo_ffa -zdo_nfa"
    CMD="$EXE $MISC -zrexExprFile:${FILE_REX} -zforced_len:400"
    #echo $CMD
    if $CMD > LOG ; then
	diff -r -w --ignore-matching-lines='Timer\|MemoryInfo' ${DIR_GOLD} ${DIR_TEST} -xlog.diff.err 2>&1 | tee log.diff.err
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
