#!/bin/bash

if [ $# != 3 ]
  then
    echo "Usage: ./runTests.sh <inputdir> <outputdir> <maxthreads>"
    exit 0
fi

if [ ! -d $1 ]
then 
    echo "Input directory must exist."
    exit 0
fi
if [ ! -d $2 ]
then 
    echo "Output directory must exist."
    exit 0
fi
if [ ! $3 -gt 0 ]
then 
    echo "Maximum number of threads must be greater than 0."
    exit 0
fi


for inputFile in $1/*
do
    for i in $(seq 1 $3)
    do  
        inputFileName=$(echo "$inputFile" | cut -f 2 -d '/')
        name=$(echo "$inputFileName"  | cut -f 1 -d '.')

        echo InputFile=$inputFileName NumThreads=$i

        ./tecnicofs $inputFile $2/$name-$i.txt $i | grep TecnicoFS
    done
done