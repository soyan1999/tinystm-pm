#!/bin/bash

bench=(
  # "bayes"
  "genome"
  "intruder"
  # "kmeans" #low contention
  # "kmeans"
  "labyrinth"
  "ssca2"
  "vacation" #low contention
  "vacation"
  "yada"
)

parameter=(
  # "-v32 -r4096 -n10 -p40 -i2 -e8 -s1 -t"
  "-g16384 -s64 -n16777216 -t"
  "-a10 -l128 -n262144 -s1 -t"
  # "-m40 -n40 -t0.00001 -i inputs/random-n65536-d32-c16 -p"
  # "-m15 -n15 -t0.00001 -i inputs/random-n65536-d32-c16 -p"
  "-i inputs/random-x512-y512-z7-n512 -t"
  "-s20 -i1.0 -u1.0 -l3 -p3 -t"
  "-n2 -q90 -u98 -r1048576 -t4194304 -c"
  "-n4 -q60 -u90 -r1048576 -t4194304 -c"
  "-a15 -i inputs/ttimeu1000000.2 -t"
  )

# scheme=(
#   "undotx"
#   "sptx_naive" )
# #  "sptx_gc"
# #)
# POOL_PATH="/mnt/pmem0/ysha/tinystm-pm/pmdk.pool"

# echo "VMMALLOC_POOL_DIR=${VMMALLOC_POOL_DIR}"
# echo "VMMALLOC_POOL_SIZE=${VMMALLOC_POOL_SIZE}"
# echo "PMEM_MMAP_HINT=${PMEM_MMAP_HINT}"
#for sequential build, use seq
result_file=$(pwd)/result.csv
ntest=${#bench[@]}
#for (( i=0; i<$ntest; i++ )); do
for (( i=0; i<$ntest; i++ )); do
  echo "**********************${bench[$i]}*****************"
  echo "${bench[$i]}, " >> $result_file
  cd ${bench[$i]}
  # rm -f compile.output
  # for j in ${scheme[@]}; do
  # make -f Makefile.ptm clean &>> compile.output
  # make -f Makefile.ptm -j &>> compile.output
  # rm $POOL_PATH
  for n in 1 2 4 8 12 16;
  # for n in 1;
  do
    echo "=========thread $n=========="
    ./${bench[$i]} ${parameter[$i]} $n | grep "Time =" | awk -F ' ' '{if($3!=""){printf ("%s, ",$3) }}' >> $result_file
    echo "============================"
  done
  echo >> $result_file
    #LD_PRELOAD=/home/ccye/vmem/src/nondebug/libvmmalloc.so ./${bench[$i]} ${parameter[$i]}
  # done
  #make -f Makefile.ptm clean &>> compile.output
  #make -f Makefile.seq -j &>> compile.output
  #echo "============================"
  #LD_PRELOAD=/home/ccye/vmem/src/nondebug/libvmmalloc.so ./${bench[$i]} ${parameter[$i]}
  
  cd ..
done
