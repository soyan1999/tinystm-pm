#!/bin/bash

FOLDERS="hashmap"

if [ $# -eq 0 ] ; then
    echo " === ERROR At the very least, we need the backend name in the first parameter. === "
    exit 1
fi

backend=$1  # e.g.: herwl


htm_retries=5
rot_retries=2

# if [ $# -eq 3 ] ; then
#     htm_retries=$2 # e.g.: 5
#     rot_retries=$3 # e.g.: 2, this can also be retry policy for tle
# fi

rm lib/*.o || true

rm Defines.common.mk
rm Makefile
rm Makefile.flags
rm lib/thread.h
rm lib/thread.c
rm lib/tm.h


cp ../backends/$backend/Defines.common.mk .
cp ../backends/$backend/Makefile .
cp ../backends/$backend/Makefile.flags .
cp ../backends/$backend/thread.h lib/
cp ../backends/$backend/thread.c lib/
cp ../backends/$backend/tm.h lib/

CPU_FREQ=$(cat CPU_FREQ_kHZ.txt | tr -d '[:space:]')
for F in $FOLDERS
do
    echo "cd $F"
    cd $F
    rm *.o || true
    rm $F # the executable
    make_command="make -f Makefile DEF_CPU_FREQ=$CPU_FREQ"
    eval $make_command
    rc=$?
    if [[ $rc != 0 ]] ; then
        echo ""
        echo "=================================== ERROR BUILDING $F - $name ===================================="
        echo ""
        exit 1
    fi
    cd ..
done

