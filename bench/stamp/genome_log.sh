#!/bin/bash

PINNING=1
SAMPLES=10

mkdir -p genome_rep
DATA=./genome_rep

cd ../../nvhtm/
make clean ; make -j40 OPTIMIZE=1 DISABLE_FLAG_IS_IN_TX=1 DISABLE_FLAG_SNAPSHOT=1 NPROFILE=1
cd -
./build-stamp.sh nvhtm

declare -A file_name=( ["LOG_REPLAY_BACKWARD"]="BACKWARD" ["LOG_REPLAY_ST_CLWB"]="ST-CLWB" \
  ["LOG_REPLAY_RANGE_FLUSHES"]="RANGE" ["LOG_REPLAY_BUFFER_WBINVD"]="BUFFER-WBINVD" \
  ["LOG_REPLAY_BUFFER_FLUSHES"]="BUFFER-FLUSHES" ["LOG_REPLAY_SYNC_SORTER"]="SYNC" \
  ["LOG_REPLAY_ASYNC_SORTER"]="ASYNC")

NB_TXS=4194304 ### default
# NB_TXS=8388608

# for s in $(seq $SAMPLES)
# do
#   # useLogicalClocks usePhysicalClocks usePCWM 
#   for sol in usePCWM
#   do
#     # LOG_REPLAY_BUFFER_WBINVD
#     for log_sol in LOG_REPLAY_BUFFER_WBINVD #LOG_REPLAY_BACKWARD
#     do
#       for t in 64
#       do
#         # LOG_REPLAY_ASYNC_SORTER
#         for NB_TXS in 1048576 #16777216 
#         do
#           echo -e "THREADS\tTHROUGHPUT\tHTM_COMMITS\tSGL_COMMITS\tHTM_ABORTS\tHTM_CONFLICT\tHTM_CAPACITY\tHTM_EXPLICIT\tHTM_OTHER" \
#             > $DATA/GENOME_${sol}-${file_name[$log_sol]}-LOGS${t}_TXS${NB_TXS}_s${s}.tsv
#           for NB_REPLAYERS in 1 2 4 8 16 32 64
#           do
#             echo "t=$t" 
#             echo "LOG_REPLAY_PARALLEL NB_REPLAYERS=${NB_REPLAYERS} PINNING=${PINNING} FLUSH_LAT=0 ${sol} ${log_sol} \
#               ERROR_FILE=$DATA/error_GENOME_${sol}-${file_name[$log_sol]}-LOGS${t}_TXS${NB_TXS}_s${s}.tsv \
#               PROFILE_FILE=$DATA/prof_GENOME_${sol}-${file_name[$log_sol]}-LOGS${t}_TXS${NB_TXS}_s${s}.tsv \
#               LOG_REPLAY_STATS_FILE=$DATA/log_GENOME_${sol}-${file_name[$log_sol]}-LOGS${t}_TXS${NB_TXS}_s${s}.tsv " \
#               > nvhtm_params.txt
#             ./genome/genome -g16384 -s64 -n$NB_TXS -t $t \
#               | sed '/^[0-9]*\t[0-9]*.[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*$/p' -n \
#               | tail -n 1 >> $DATA/GENOME_${sol}-${file_name[$log_sol]}-LOGS${t}_TXS${NB_TXS}_s${s}.tsv
#             sleep 0.05s
#           done
#         done
#       done
#     done
#   done
# done

for s in $(seq $SAMPLES)
do
  # useLogicalClocks usePhysicalClocks usePCWM 
  for sol in usePCWM2 #usePCWM3
  do
    # LOG_REPLAY_BUFFER_WBINVD
    for log_sol in LOG_REPLAY_BUFFER_WBINVD
    do
      for t in 64
      do
        # LOG_REPLAY_ASYNC_SORTER
        for NB_TXS in 1048576 #16777216
        do
          echo -e "THREADS\tTHROUGHPUT\tHTM_COMMITS\tSGL_COMMITS\tHTM_ABORTS\tHTM_CONFLICT\tHTM_CAPACITY\tHTM_EXPLICIT\tHTM_OTHER" \
            > $DATA/GENOME_${sol}-${file_name[$log_sol]}-LOGS${t}_TXS${NB_TXS}_s${s}.tsv
          for NB_REPLAYERS in 1 2 4 8 16 32 64
          do
            echo "t=$t" 
            echo "LOG_REPLAY_PARALLEL NB_REPLAYERS=${NB_REPLAYERS} PINNING=${PINNING} FLUSH_LAT=0 ${sol} ${log_sol} \
              ERROR_FILE=$DATA/error_GENOME_${sol}-${file_name[$log_sol]}-LOGS${t}_TXS${NB_TXS}_s${s}.tsv \
              PROFILE_FILE=$DATA/prof_GENOME_${sol}-${file_name[$log_sol]}-LOGS${t}_TXS${NB_TXS}_s${s}.tsv \
              LOG_REPLAY_STATS_FILE=$DATA/log_GENOME_${sol}-${file_name[$log_sol]}-LOGS${t}_TXS${NB_TXS}_s${s}.tsv " \
              > nvhtm_params.txt
            ./genome/genome -g16384 -s64 -n$NB_TXS -t $t \
              | sed '/^[0-9]*\t[0-9]*.[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*$/p' -n \
              | tail -n 1 >> $DATA/GENOME_${sol}-${file_name[$log_sol]}-LOGS${t}_TXS${NB_TXS}_s${s}.tsv
            sleep 0.05s
          done
        done
      done
    done
  done
done
