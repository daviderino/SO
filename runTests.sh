#!/bin/bash

INPUTDIR="$1"
OUTPUTDIR="$2"
MAXTHREADS="$3"
NUMBUCKETS="$4"

NOSYNC="tecnicofs-nosync"
MUTEX="tecnicofs-mutex"

for file_in in `ls -rS ${INPUTDIR}/*.txt`; do
	f=${file_in::-4}
    file_out="${OUTPUTDIR}/${f#*/}"
	echo -e "InputFile=${file_in#*/} NumThreads=1"
	(./${NOSYNC} ${file_in} ${file_out}-1.txt 1 ${NUMBUCKETS}) | grep "TecnicoFS completed"
	for i in `seq 2 $MAXTHREADS`; do
		echo -e "InputFile=${file_in#*/} NumThreads=${i}"
		(./${MUTEX} ${file_in} ${file_out}-${i}.txt ${i} ${NUMBUCKETS}) | grep "TecnicoFS completed"
	done
done
