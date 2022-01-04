backend=$1 # e.g: nvhtm

cp ../backends/$backend/tm.h ./include/
cp ../backends/$backend/thread.c ./src/
cp ../backends/$backend/thread.h ./include/
cp ../backends/$backend/Makefile .
# cp ../backends/$backend/Makefile.common .
cp ../backends/$backend/Makefile.flags .
cp ../backends/$backend/Defines.common.mk .

rm $(find . -name *.o)

cd code;
rm tpcc

CPU_FREQ=$(cat CPU_FREQ_kHZ.txt | tr -d '[:space:]')
make_command="make -j8 -f Makefile DEF_CPU_FREQ=$CPU_FREQ $MAKEFILE_ARGS"
echo " ==========> $make_command"
$make_command
