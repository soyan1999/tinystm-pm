backend=$1 # e.g: nvhtm

cp ../backend/$backend/tm.h ./include/
cp ../backend/$backend/thread.c ./src/
cp ../backend/$backend/thread.h ./include/
cp ../backend/$backend/Makefile .
# cp ../backend/$backend/Makefile.common .
cp ../backend/$backend/Makefile.flags .
cp ../backend/$backend/Defines.common.mk .

rm $(find . -name *.o)

cd code;
rm tpcc

CPU_FREQ=$(cat CPU_FREQ_kHZ.txt | tr -d '[:space:]')
make_command="make -j8 -f Makefile DEF_CPU_FREQ=$CPU_FREQ $MAKEFILE_ARGS"
echo " ==========> $make_command"
$make_command
