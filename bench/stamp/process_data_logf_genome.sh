source ../../paths.sh

if [[ $# -gt 0 ]] ; then
	NODE=$1
fi

SCRIPT_PATH=$(pwd)/../..
SCRIPT_AVG=$SCRIPT_PATH/SCRIPT_compute_AVG_ERR.R

SCRIPT_THROUGHPUT_PLOT=$SCRIPT_PATH/plot_throughput.py
# SCRIPT_THROUGHPUT_PLOT=$SCRIPT_PATH/plot_throughput_v2.py
SCRIPT_ABORT_PLOT=$SCRIPT_PATH/plot_aborts.py
SCRIPT_PROF_PLOT=$SCRIPT_PATH/plot_prof.py
SCRIPT_LOG_PLOT=$SCRIPT_PATH/plot_log.py
SCRIPT_LOG_REP_PLOT=$SCRIPT_PATH/bench/stamp/plot_log_replayers.py
SCRIPT_LOG_PROF_PLOT=$SCRIPT_PATH/plot_prof_log.py
SCRIPT_LOG_PROF_2=$SCRIPT_PATH/plot_prof_log_2.py
SCRIPT_LOG_BYTES_PLOT=$SCRIPT_PATH/plot_bytes_log.py
SCRIPT_COL=$SCRIPT_PATH/SCRIPT_put_column.R

cd $DATA_PATH
mkdir -p genome_rep
cd genome_rep
EXPERIMENT_FOLDER=$(date +%Y-%m-%dT%H_%m_%S)
mkdir -p $EXPERIMENT_FOLDER
cd $EXPERIMENT_FOLDER
mkdir -p data
scp $NODE:~/$DM/bench/stamp/genome_rep/* data/
scp $NODE:~/$DM/bench/stamp/vacation_rep/* data/

$SCRIPT_COL "THROUGHPUT = ((a\$WRITTEN_ENTRIES - a\$TOTAL_NB_TXS) / 2) / (a\$TIME_TOTAL / 2300000000); a = cbind(a, THROUGHPUT); " \
  $(ls data/log*PCWM-*.tsv)
$SCRIPT_COL "THROUGHPUT = ((a\$WRITTEN_ENTRIES - (2 * a\$TOTAL_NB_TXS)) / 2) / (a\$TIME_TOTAL / 2300000000); a = cbind(a, THROUGHPUT); " \
  $(ls data/log*PCWM2*.tsv)

MALLOC_SIZE=536870912

# usePCWM2
for sol in  usePCWM2 # usePCWM3
do
  # BACKWARD NORMAL  BUFFER-FLUSHES
  for log_sol in BUFFER-WBINVD
  do
    for NB_TXS in 1048576 #8388608 16777216 33554432
    do
      $SCRIPT_AVG $(ls data/log_GENOME_${sol}-${log_sol}-LOGS64_TXS${NB_TXS}_s*.tsv.cols)
      mv avg.txt log_GENOME_${sol}-${log_sol}-LOGS64_TXS${NB_TXS}_avg.tsv
      mv stdev.txt log_GENOME_${sol}-${log_sol}-LOGS64_TXS${NB_TXS}_stdev.tsv

      touch GENOME_${sol}-${log_sol}-LOGS64_TXS${NB_TXS}_avg.tsv
      touch GENOME_${sol}-${log_sol}-LOGS64_TXS${NB_TXS}_stdev.tsv
    done
  done
done

for sol in usePCWM
do
  # BACKWARD BUFFER-WBINVD  BUFFER-FLUSHES
  for NB_TXS in 1048576 #8388608 16777216 33554432
  do
    for log_sol in BUFFER-WBINVD # BACKWARD
    do
      $SCRIPT_AVG $(ls data/log_GENOME_${sol}-${log_sol}-LOGS64_TXS${NB_TXS}_s*.tsv.cols)
      mv avg.txt log_GENOME_${sol}-${log_sol}-LOGS64_TXS${NB_TXS}_avg.tsv
      mv stdev.txt log_GENOME_${sol}-${log_sol}-LOGS64_TXS${NB_TXS}_stdev.tsv

      touch GENOME_${sol}-${log_sol}-LOGS64_TXS${NB_TXS}_avg.tsv
      touch GENOME_${sol}-${log_sol}-LOGS64_TXS${NB_TXS}_stdev.tsv
    done
  done
done

#### added Vacation also

# usePCWM2
for sol in  usePCWM2 # usePCWM3
do
  # BACKWARD NORMAL  BUFFER-FLUSHES
  for log_sol in BUFFER-WBINVD
  do
    for NB_TXS in 4194304 #8388608 16777216 33554432
    do
      $SCRIPT_AVG $(ls data/log_VACATION_${sol}-${log_sol}-LOGS64_TXS${NB_TXS}_s*.tsv.cols)
      mv avg.txt log_VACATION_${sol}-${log_sol}-LOGS64_TXS${NB_TXS}_avg.tsv
      mv stdev.txt log_VACATION_${sol}-${log_sol}-LOGS64_TXS${NB_TXS}_stdev.tsv

      touch VACATION_${sol}-${log_sol}-LOGS64_TXS${NB_TXS}_avg.tsv
      touch VACATION_${sol}-${log_sol}-LOGS64_TXS${NB_TXS}_stdev.tsv
    done
  done
done

for sol in usePCWM
do
  # BACKWARD BUFFER-WBINVD  BUFFER-FLUSHES
  for NB_TXS in 4194304 #8388608 16777216 33554432
  do
    for log_sol in BUFFER-WBINVD # BACKWARD
    do
      $SCRIPT_AVG $(ls data/log_VACATION_${sol}-${log_sol}-LOGS64_TXS${NB_TXS}_s*.tsv.cols)
      mv avg.txt log_VACATION_${sol}-${log_sol}-LOGS64_TXS${NB_TXS}_avg.tsv
      mv stdev.txt log_VACATION_${sol}-${log_sol}-LOGS64_TXS${NB_TXS}_stdev.tsv

      touch VACATION_${sol}-${log_sol}-LOGS64_TXS${NB_TXS}_avg.tsv
      touch VACATION_${sol}-${log_sol}-LOGS64_TXS${NB_TXS}_stdev.tsv
    done
  done
done




# tar -Jcf data.tar.xz ./data
# rm -r ./data
echo " >>>> data is in $PWD"

# for lat in 500
# do
#   $SCRIPT_THROUGHPUT_PLOT "lat${lat}ns" $(ls ${lat}_*)
#   $SCRIPT_ABORT_PLOT "lat${lat}ns" $(ls ${lat}_*)
#   $SCRIPT_PROF_PLOT "lat${lat}ns" $(ls prof_${lat}_*)
# done

#################


# $SCRIPT_THROUGHPUT_PLOT "test_log" $(ls use*)

$SCRIPT_LOG_REP_PLOT "4194304" $(ls GENOME*PCWM*1048576*) $(ls VACATION*PCWM*4194304*)

# $SCRIPT_LOG_REP_PLOT "8388608" $(ls VACATION*PCWM*8388608*)
# $SCRIPT_LOG_REP_PLOT "16777216" $(ls VACATION*PCWM*16777216*)
# $SCRIPT_LOG_REP_PLOT "33554432" $(ls VACATION*PCWM*33554432*)

# $SCRIPT_LOG_REP_PLOT "WBINVD" $(ls use*WBINVD*)
# $SCRIPT_LOG_PROF_PLOT "test_log" $(ls log_use*)
# $SCRIPT_LOG_BYTES_PLOT "test_log" $(ls log_use*)

# $SCRIPT_LOG_PROF_2 "profiling" $(ls log_usePhysicalClocks-BUFFER-WBINVD-SYNC-SORTER*) \
#   $(ls log_usePhysicalClocks*-FLUSHES-* log_usePhysicalClocks*-BACKWARD-* log_usePhysicalClocks*-RANGE-* \
#   log_usePCWM2*-WBINVD-*)

# $SCRIPT_LOG_PROF_PLOT "all_stacked" $(ls log_usePhysicalClocks-BUFFER-WBINVD-SYNC-SORTER*) \
#   $(ls log_usePhysicalClocks*-FLUSHES-*SYNC* log_usePhysicalClocks*-BACKWARD-*SYNC*)

cd $SCRIPT_PATH
