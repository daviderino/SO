#!/bin/bash

if [ $# -ne 4 ] ; then
	echo "Invalid number of arguments"
	echo "Example: ./runTests.sh inputdir outputdir maxthreads numbuckets"
	exit 1
fi

INPUTDIR="$1"
OUTPUTDIR="$2"
MAXTHREADS="$3"
NUMBUCKETS="$4"

regex='^[0-9]+$'

if ! [[ $3 =~ $regex ]]
then
	echo "Number of threads must be an integer"
	exit 1
fi

if ! [[ $4 =~ $regex ]]
then
	echo "Number of buckets must be an integer"
	exit 1
fi

if [ ! -d $1 ]
then
	echo "Input folder non-existent"
	exit 1
fi


if [ ! -d $2 ]
then
	echo "Output folder non-existent"
	exit 1
fi


if (($3 <= 0))
then
	echo "Number of threads should be positive"
	exit 1
fi

if (($4 <= 0))
then
	echo "Number of buckets should be positive"
	exit 1
fi

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
