#!/bin/bash

bench=(
  "hashmap"
  "btree"
  "rbtree_"
  # "simple_array"
)

parameter=(
  "-u 100 -d 10000000 -n"
  "-u 100 -d 10000000 -n"
  "-u 100 -d 10000000 -n"
  # "-u 100 -d 10000000 -n"
  )

NB_SAMPLES=10
TIME_MEASURE=0

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
  # for n in 2;
  do
    echo "=========thread $n=========="

    val=0
    for s in $(seq $NB_SAMPLES)
    do
      timeout 20m ./${bench[$i]} ${parameter[$i]} $n > out.txt
      cat out.txt
      if [ $TIME_MEASURE -eq 0 ]; then
        res=$(cat out.txt | grep "Time =" | awk -F ' ' '{if($3!=""){printf ("%s",$3) }}')
      else
        res=$(../avg.py time_measure_0.txt)
      fi
      val=$(echo "scale=6; $val+$res" | bc)
      # ./${bench[$i]} ${parameter[$i]} $n | grep "Time =" | awk -F ' ' '{if($3!=""){printf ("%s, ",$3) }}' >> $result_file
    done
    val=$(echo "scale=6; $val/$NB_SAMPLES" | bc)
    echo -n "$val, " >> $result_file
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
