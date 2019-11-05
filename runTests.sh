#!/bin/bash

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m'

if [ $# -ne 4 ] ; then
	echo -e "${RED}Invalid number of arguments ${NC}"
	echo -e "${BLUE}Example: ./runTests.sh inputdir outputdir maxthreads numbuckets ${NC}"
	exit 1
fi

INPUTDIR="$1"
OUTPUTDIR="$2"
MAXTHREADS="$3"
NUMBUCKETS="$4"

NOSYNC="tecnicofs-nosync"
MUTEX="tecnicofs-mutex"

GOOD=1
for file_in in `ls -rS ${INPUTDIR}/*.txt`; do
    file_out="${OUTPUTDIR}/${file_in#*/}"
	echo "OUTPUT FILE ${file_out}"
	if [${MAXTHREADS} == 1]; then
		echo -e "InputFile=${file_in} NumThreads=1"
		(./${NOSYNC} ${file_in} ${file_out} 1 ${NUMBUCKETS})
	else
		for ((i=2;i<=MAXTHREADS;i++)); do
			echo -e "InputFile=${file_in} NumThreads=${i}"
			(./${MUTEX} ${file_in} ${file_out} ${i} ${NUMBUCKETS})
		done
	fi
done

if [ ${GOOD} == 1 ]; then
    echo -e "${GREEN}+++++++++++++++++"
    echo "Passed all tests"
    echo -e "+++++++++++++++++${NC}"
fi
