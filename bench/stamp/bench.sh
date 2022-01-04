#!/bin/bash

source benches_args.sh

cd ../../nvhtm
make clean
### TODO!!!
make -j40 OPTIMIZE=1 DISABLE_FLAG_IS_IN_TX=1 NPROFILE=1
cd -

### this file must exist here
cp ../../nvhtm/CPU_FREQ_kHZ.txt .

./build-stamp.sh nvhtm
mkdir -p ./data

NB_SAMPLES=10
PINNING=1

NB_BENCHES=1

for i in $(seq $NB_BENCHES)
do
  # useEpochCommit1 --> blocks
  # useHTM useLogicalClocks usePhysicalClocks usePCWM2 usePCWM useCcHTM 
  #usePHTM useHTM useLogicalClocks usePhysicalClocks usePCWM2 usePCWM 
  # usePCWM usePCWM2 usePHTM useLogicalClocks usePhysicalClocks
  for s in $(seq $NB_SAMPLES)
  do
    # useLogicalClocks usePhysicalClocks usePCWM usePCWM2 usePCWM3 useCcHTMbest 
    # for sol in usePCWM usePCWM2 usePCWM3  
    for sol in usePCWM usePCWM2 usePCWM3 usePhysicalClocks useCcHTMbest
    do
    #LOG_REPLAY_BUFFER_WBINVD
      echo "DISABLE_LOG_REPLAY PINNING=${PINNING} FLUSH_LAT=0 ${sol}=1 \
        ERROR_FILE=./data/err${test_name[$i]}_${sol}_s${s}.tsv \
        PROFILE_FILE=./data/prof${test_name[$i]}_${sol}_s${s}.tsv " \
        > nvhtm_params.txt
      echo -e "THREADS\tTIME\tNB_HTM_SUCCESS\tNB_FALLBACK\tNB_ABORTS\tNB_CONFL\tNB_CAPAC\tNB_EXPLI\tNB_OTHER" \
        > data/${test_name[$i]}_${sol}_s${s}.tsv
      #  40 48 56 64
      for n in 1 2 4 8 12 16 20 24 28 32 33 40 48 64
      do
        echo "${test[$i]} $n"
        ### TODO: this is not bullet proof! would be better to produce a file than stdout
        timeout 20m ${test[$i]} $n > out.txt
        cat out.txt | head
        cat out.txt | tail
        cat out.txt | sed '/^[0-9]*\t[0-9]*.[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*$/p' -n  | tail -n 1 \
          >> data/${test_name[$i]}_${sol}_s${s}.tsv
        echo $(tail -n 1 data/${test_name[$i]}_${sol}_s${s}.tsv)
        pkill "${test[$i]} $n"
      done
    done
  done
done

# for i in $(seq $NB_BENCHES)
# do
#   # useEpochCommit1 --> blocks
#   # useHTM useLogicalClocks usePhysicalClocks usePCWM2 usePCWM useCcHTM 
#   #usePHTM useHTM useLogicalClocks usePhysicalClocks usePCWM2 usePCWM 
#   # usePCWM usePCWM2 usePHTM useLogicalClocks usePhysicalClocks
#     # useLogicalClocks usePhysicalClocks usePCWM usePCWM2 usePCWM3 useCcHTMbest 
#   for s in $(seq $NB_SAMPLES)
#   do
#     for sol in useCrafty
#     do
#     #LOG_REPLAY_BUFFER_WBINVD
#       echo "DISABLE_LOG_REPLAY PINNING=${PINNING} FLUSH_LAT=0 ${sol}=1 \
#         ERROR_FILE=./data/err${test_name[$i]}_${sol}_s${s}.tsv \
#         PROFILE_FILE=./data/prof${test_name[$i]}_${sol}_s${s}.tsv " \
#         > nvhtm_params.txt
#       echo -e "THREADS\tTIME\tNB_HTM_SUCCESS\tNB_FALLBACK\tNB_ABORTS\tNB_CONFL\tNB_CAPAC\tNB_EXPLI\tNB_OTHER" \
#         > data/${test_name[$i]}_${sol}_s${s}.tsv
#       #  40 48 56 64
#       for n in 1 2 4 8 12 16 20 24 28 32 33 40 48 64
#       do
#         echo "${test[$i]} $n"
#         ### TODO: this is not bullet proof! would be better to produce a file than stdout
#         timeout 20m ${test[$i]} $n > out.txt
#         cat out.txt | head
#         cat out.txt | tail
#         cat out.txt | sed '/^[0-9]*\t[0-9]*.[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*$/p' -n  | tail -n 1 \
#           >> data/${test_name[$i]}_${sol}_s${s}.tsv
#         echo $(tail -n 1 data/${test_name[$i]}_${sol}_s${s}.tsv)
#         pkill "${test[$i]} $n"
#       done
#     done
#   done
# done

# cd ../../deps/tinystm ; make clean ; make ; cd - ; ./build-stamp.sh pstm
# mkdir -p ./data

# for s in $(seq $NB_SAMPLES)
# do
#   # useEpochCommit1 --> blocks
#   # useHTM useLogicalClocks usePhysicalClocks usePCWM2 usePCWM useCcHTM 
#   #usePHTM useHTM useLogicalClocks usePhysicalClocks usePCWM2 usePCWM 
#   # usePCWM usePCWM2 usePHTM useLogicalClocks usePhysicalClocks
#   for i in $(seq $NB_BENCHES)
#   do
#     for sol in usePSTM
#     do
#     #LOG_REPLAY_BUFFER_WBINVD
#       echo "DISABLE_LOG_REPLAY PINNING=${PINNING} FLUSH_LAT=0 ${sol}=1 \
#         ERROR_FILE=./data/err${test_name[$i]}_${sol}_s${s}.tsv \
#         PROFILE_FILE=./data/prof${test_name[$i]}_${sol}_s${s}.tsv " \
#         > nvhtm_params.txt
#       echo -e "THREADS\tTIME\tCOMMITS\tABORTS" \
#         > data/${test_name[$i]}_${sol}_s${s}.tsv
#       #  40 48 56 64
#       for n in 1 2 4 8 12 16 20 24 28 32 33 40 48 64
#       do
#         echo "${test[$i]} $n"
#         ### TODO: this is not bullet proof! would be better to produce a file than stdout
#         ${test[$i]} $n > out.txt
#         cat out.txt | head
#         cat out.txt | tail
#         cat out.txt | sed '/^[0-9]*\t[0-9]*.[0-9]*\t[0-9]*\t[0-9]*$/p' -n  | tail -n 1 \
#           >> data/${test_name[$i]}_${sol}_s${s}.tsv
#         echo $(tail -n 1 data/${test_name[$i]}_${sol}_s${s}.tsv)
#       done
#     done
#   done
# done

echo "DISABLE_LOG_REPLAY PINNING=1 FLUSH_LAT=0 usePCWM=1 \
        ERROR_FILE=error_file \
        PROFILE_FILE=prof_file " \
        > nvhtm_params.txt

# cd ../tpcc

# ./bench.sh
