#!/bin/bash
INPUT=$1
OUTPUT=$2
THREADS=$3
NUMBUCKETS=$4
STRINGTRACO="-"
mkdir ${OUTPUT} 2>/dev/null

if [ ! $# -eq 4 ]; then
    echo "Numero de argumentos errado"
    exit 0
fi

if [ ! -d "$INPUT" ]; then
    echo "Diretoria para input nao especificada"
    exit 0
fi

if [ ! -d "$OUTPUT" ]; then
    echo "Diretoria para output nao especificada"
    exit 0
fi

if [ $THREADS -lt 1 ]; then
    echo "Numero de threads abaixo de 1"
    exit 0
fi

if [ $NUMBUCKETS -lt 1 ]; then
    echo "Numero de buckets abaixo de 1"
    exit 0
fi

for i in $(cd $INPUT; ls)
do
    echo "InputFile=$i NumThreads=1"
    ./tecnicofs-nosync $1/$i $OUTPUT/$i$STRINGTRACO"1" 1 1 | grep "TecnicoFS"
    for x in $(seq 2 ${THREADS})
    do
        echo "InputFile=$i NumThreads=${x}"
        ./tecnicofs-mutex $1/$i $OUTPUT/$i$STRINGTRACO$x $x $NUMBUCKETS | grep "TecnicoFS"
    done
done

