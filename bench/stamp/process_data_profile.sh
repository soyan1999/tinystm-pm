source ../../paths.sh

NB_BENCHES=3

test[1]="./vacation/vacation -n2 -q90 -u98 -r1048576 -t4194304 -c"  # low
test[2]="./intruder/intruder -a10 -l128 -n262144 -s1 -t"
test[3]="./yada/yada -a15 -i ./yada/inputs/ttimeu1000000.2 -t"

test_name[1]="VACATION_LOW"
test_name[2]="INTRUDER"
test_name[3]="YADA"

if [[ $# -gt 0 ]] ; then
	NODE=$1
fi

SCRIPT_PATH=$(pwd)
SCRIPT_PROF_MULTIPLOT=$SCRIPT_PATH/plot_prof_multi.py
SCRIPT_ABORT_MULTIPLOT=$SCRIPT_PATH/plot_aborts_multi.py
SCRIPT_ABORT_PLOT=$SCRIPT_PATH/plot_aborts.py
SCRIPT_PROF_PLOT=$SCRIPT_PATH/plot_prof.py
SCRIPT_AVG=$SCRIPT_PATH/../../SCRIPT_compute_AVG_ERR.R
SCRIPT_COL=$SCRIPT_PATH/../../SCRIPT_put_column.R

cd $DATA_PATH
mkdir -p stamp_prof
cd stamp_prof
EXPERIMENT_FOLDER=$(date +%Y-%m-%dT%H_%m_%S)
mkdir -p $EXPERIMENT_FOLDER
cd $EXPERIMENT_FOLDER
mkdir -p data
scp $NODE:~/$DM/bench/stamp/data_prof/* data/

for b in $(seq $NB_BENCHES)
do
  for sol in useCcHTMbest usePhysicalClocks useLogicalClocks usePCWM usePCWM2 usePCWM3 useCrafty
  do
    for f in $(ls data/${test_name[$b]}_${sol}_s*)
    do
      sed "s/ /$(printf '\t')/g" $f > $f.out
    done
    # $SCRIPT_COL "SUM = (a\$TX_TIME + a\$COMMIT_TIME + a\$NON_TX_TIME); \
    # P_TX = (a\$TX_TIME) / (a\$TXS / a\$THREADS); \
    # P_COM = (a\$COMMIT_TIME) / (a\$TXS / a\$THREADS);\
    # P_NTX = (a\$NON_TX_TIME) / (a\$TXS / a\$THREADS); \
    # a = cbind(a, P_TX, P_COM, P_NTX); " \
    #   $(ls data/${test_name[$b]}_${sol}_s*.out)
  done
done


# for sol in useCcHTMbest usePhysicalClocks useLogicalClocks usePCWM usePCWM2 usePCWM3 useCrafty
# do
#   for i in $(seq $NB_BENCHES)
#   do
#     $SCRIPT_AVG $(ls data/${test_name[$i]}_${sol}_s*.out.cols)
#     echo "${test_name[$i]}_${sol}"
#     mv avg.txt ${test_name[$i]}_${sol}_avg.tsv
#     mv stdev.txt ${test_name[$i]}_${sol}_stdev.tsv
#     # $SCRIPT_AVG $(ls data/prof${test_name[$i]}_${sol}_s*.tsv)
#     # mv avg.txt prof${test_name[$i]}_${sol}_avg.tsv
#     # mv stdev.txt prof${test_name[$i]}_${sol}_stdev.tsv
#   done
# done

for b in $(seq $NB_BENCHES)
do
  for sol in useCcHTMbest usePhysicalClocks useLogicalClocks usePCWM usePCWM2 usePCWM3 useCrafty
  do
    $SCRIPT_AVG $(ls data/${test_name[$b]}_${sol}_s*.out)
    echo "${test_name[$b]}_${sol}"
    mv avg.txt ${test_name[$b]}_${sol}_avg.tsv
    mv stdev.txt ${test_name[$b]}_${sol}_stdev.tsv
    # $SCRIPT_AVG $(ls data/prof${test_name[$i]}_${sol}_s*.tsv)
    # mv avg.txt prof${test_name[$i]}_${sol}_avg.tsv
    # mv stdev.txt prof${test_name[$i]}_${sol}_stdev.tsv
  done
done

# tar -Jcf data.tar.xz ./data
# rm -r ./data
echo " >>>>> data is in $PWD >>>>>"

# NB_SOLUTIONS=7
# $SCRIPT_PROF_MULTIPLOT 3 \
#   $NB_SOLUTIONS $(ls VACATION_LOW_use*) \
#   $NB_SOLUTIONS $(ls INTRUDER_use*) \
#   $NB_SOLUTIONS $(ls YADA_use*)

# NB_SOLUTIONS=7
# $SCRIPT_PROF_MULTIPLOT 3 \
#   $NB_SOLUTIONS $(ls VACATION_LOW_use*) \
#   $NB_SOLUTIONS $(ls INTRUDER_use*) \
#   $NB_SOLUTIONS $(ls YADA_use*) 

NB_SOLUTIONS=7
$SCRIPT_PROF_MULTIPLOT 2 \
  $NB_SOLUTIONS $(ls VACATION_LOW_use*)\
  $NB_SOLUTIONS $(ls INTRUDER_use*)



# cd $SCRIPT_PATH
