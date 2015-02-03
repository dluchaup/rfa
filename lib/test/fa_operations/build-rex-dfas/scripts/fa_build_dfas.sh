#!/bin/bash
SCRIPTDIR=$(dirname $(readlink -f $0) )
BASE=$(dirname $SCRIPTDIR)
#echo $BASE; exit 0
EXE=$BASE/../../../../tools/build-rex-dfas/bin/build-rex-dfas
DIR_TEST=$BASE/tmp
DIR_GOLD=$BASE/GOLD
DIR_DATA=$BASE/data
#CHECKER=valgrind --leak-check=full
#MISC="$CHECKER"

if [ ! -f ${EXE} ] ; then
    make -C $BASE/../../../../tools/build-rex-dfas
    if [ ! -f ${EXE} ] ; then
	echo "cannot find or build " ${EXE}
	exit 1;
    fi
fi

if [ ! -d tmp ] ; then
    mkdir tmp
fi

if [ ! -d ${DIR_TEST} ] ; then
    mkdir ${DIR_TEST}
fi

# if [ ! -d ${DIR_GOLD} ] ; then
#     pushd $BASE
#     tar xf GOLD.tar.bz2
#     popd
# fi

if [ -d ${DIR_TEST} ] ; then
    rm ${DIR_TEST}/* 2> /dev/null
    pushd ${DIR_TEST}
    CMD="$MISC $EXE -zsigFile:${DIR_DATA}/http-1400.sorted.txt -z dumpSigMdfa:aaa"
    $CMD > /dev/null
    popd;
    diff -r -w --ignore-matching-lines='Timer\|->unparse()=' ${DIR_GOLD} ${DIR_TEST} -xlog.diff.err 2>&1 > ${DIR_TEST}/log.diff.err
    if [[ ! -s ${DIR_TEST}/log.diff.err ]] ; then # i.e. if log.diff.err is not empty
	echo "PASSED $(basename $0)"
    else
	echo "DIFFERENT $(basename $0) FAILED"
    fi
else
    echo cannot find TST directory ${DIR_TEST}
fi
