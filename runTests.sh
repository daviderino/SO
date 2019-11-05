#!/bin/bash
if [ $# -lt 2 ] ; then
    echo "Example: ./runTests.sh inputdir outputdir maxthreads numbuckets"
    exit 1
fi
