source ../../paths.sh
source benches_args.sh

if [[ $# -gt 0 ]] ; then
	NODE=$1
fi

SCRIPT_PATH=$(pwd)
SCRIPT_THROUGHPUT_PLOT=$SCRIPT_PATH/plot_throughput.py
SCRIPT_ABORT_PLOT=$SCRIPT_PATH/plot_aborts.py
SCRIPT_PROF_PLOT=$SCRIPT_PATH/plot_prof.py
SCRIPT_AVG=$SCRIPT_PATH/../../SCRIPT_compute_AVG_ERR.R

cd $DATA_PATH
mkdir -p tpcc
cd tpcc
EXPERIMENT_FOLDER=$(date +%Y-%m-%dT%H_%m_%S)
mkdir -p $EXPERIMENT_FOLDER
cd $EXPERIMENT_FOLDER
mkdir -p data
scp $NODE:~/$DM/bench/tpcc/data/* data/

### TODO: this is copy from mini_bench
# useEpochCommit2 --> is too bad
# usePhysicalClocks useLogicalClocks useEpochCommit1
# usePCWC useFastPCWC 
#usePCWC-F usePCWC-NF usePCWC-F-DA usePCWC-NF-DA usePCWC-F-DSN usePCWC-NF-DSN usePCWC-F-DTX usePCWC-NF-DTX usePCWC-F-DWC usePCWC-NF-DWC
for sol in useCcHTMbest usePhysicalClocks useLogicalClocks usePCWM usePCWM2 usePCWM3 useCrafty usePSTM
do
  for i in $(seq $NB_BENCHES)
  do
    $SCRIPT_AVG $(ls data/${test_name[$i]}_${sol}_s*.tsv)
    mv avg.txt ${test_name[$i]}_${sol}_avg.tsv
    mv stdev.txt ${test_name[$i]}_${sol}_stdev.tsv
    $SCRIPT_AVG $(ls data/prof${test_name[$i]}_${sol}_s*.tsv)
    mv avg.txt prof${test_name[$i]}_${sol}_avg.tsv
    mv stdev.txt prof${test_name[$i]}_${sol}_stdev.tsv
  done
done

# tar -Jcf data.tar.xz ./data
# rm -r ./data
echo "data is in $PWD"

for i in $(seq $NB_BENCHES)
do
  $SCRIPT_THROUGHPUT_PLOT "${test_name[$i]}" $(ls ${test_name[$i]}_*)
  $SCRIPT_ABORT_PLOT "${test_name[$i]}" $(ls ${test_name[$i]}_*)
  $SCRIPT_PROF_PLOT "${test_name[$i]}" $(ls prof${test_name[$i]}_*)
done

cd $SCRIPT_PATH
