#!/bin/bash

FOLDER=data_prof

mkdir $FOLDER

### TODO: do a better script

######################################################
NB_BENCHES=3

test[1]="./vacation/vacation -n2 -q90 -u98 -r1048576 -t4194304 -c"  # low
test[2]="./intruder/intruder -a10 -l128 -n262144 -s1 -t"
test[3]="./yada/yada -a15 -i ./yada/inputs/ttimeu1000000.2 -t"

test_name[1]="VACATION_LOW"
test_name[2]="INTRUDER"
test_name[3]="YADA"



for i in $(seq $NB_BENCHES)
do
  for s in $(seq 5)
  do

    for sol in usePCWM useCrafty usePCWM2 usePCWM3 useCcHTMbest useLogicalClocks usePhysicalClocks
    # for sol in useCrafty
    do

      echo "DISABLE_LOG_REPLAY PINNING=1 FLUSH_LAT=0 $sol=1         ERROR_FILE=error_file         PROFILE_FILE=prof_file   " > nvhtm_params.txt
      rm prof_file
      echo "THREADS	TIME	NB_HTM_SUCCESS	NB_FALLBACK	NB_ABORTS	NB_CONFL	NB_CAPAC	NB_EXPLI	NB_OTHER" > run.out
      for t in 1 4 16 32 64
      do
        timeout 20m ${test[$i]} $t > out.txt
        cat out.txt | sed '/^[0-9]*\t[0-9]*.[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*$/p' -n | tail -n 1 >> run.out
      done
      cut -f1,2,3,4,5 run.out > bench1.out
      cut -f1,2,3,4,5,7,8,9 prof_file > bench2.out
      echo "THREADS	TXS	TX_TIME	COMMIT_TIME	NON_TX_TIME	SGL_TIME	TX_FAIL_TIME	COMMIT_FAIL_TIME	COMMITS_HTM	COMMITS_SGL	ABORTS" > $FOLDER/${test_name[$i]}_${sol}_s${s}
      paste bench1.out bench2.out | sed 1d | \
        awk '{print $1,$7,(($10/$1)/2300000000),(($9/$1)/2300000000),($2-((($9+$10)/$1)/2300000000)),((($11)/$1)/2300000000),((($12)/$1)/2300000000),((($13)/$1)/2300000000),$3,$4,$5}' >> $FOLDER/${test_name[$i]}_${sol}_s${s}
    done

  done
  
done

# for s in $(seq 10)
# do

#   for i in $(seq $NB_BENCHES)
#   do

#     for sol in usePSTM 
#     do

#       echo "DISABLE_LOG_REPLAY PINNING=1 FLUSH_LAT=0 $sol=1         ERROR_FILE=error_file         PROFILE_FILE=prof_file" > nvhtm_params.txt

#       echo "THREADS	TIME	NB_HTM_SUCCESS	NB_FALLBACK	NB_ABORTS	NB_CONFL	NB_CAPAC	NB_EXPLI	NB_OTHER" > run.out
#       for i in 1 4 16 32 64
#       do
#         ${test[$i]} | \
#         sed '/^[0-9]*\t[0-9]*.[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*$/p' -n  | \
#         tail -n 1 >> run.out
#       done
#       cut -f1,2 run.out > bench1.out
#       cut -f1,2,3,4,5 prof_file > bench2.out
#       echo "TX_TIME	COMMIT_TIME	NON_TX_TIME" > $FOLDER/${test_name[$i]}_s${i}
#       paste bench1.out bench2.out | sed 1d | awk '{print (($7/$1)/2300000000),(($6/$1)/2300000000),($2-(($5/$1)/2300000000))}' >> $FOLDER/VACATION_LOW_s${i}
#     done

#   done

# done


######################################################



