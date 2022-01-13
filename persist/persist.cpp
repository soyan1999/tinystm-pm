#include "persist.h"
#include "rdtsc.h"

__thread unsigned long pstm_time_flush_redo_log = 0;
__thread unsigned long pstm_time_flush_data = 0;
__thread unsigned long pstm_time_tx = 0;
__thread unsigned long pstm_size_flush = 0;
__thread unsigned long pstm_nb_tx = 0;
__thread unsigned long pstm_nb_flush = 0;
volatile unsigned long tot_pstm_time_flush_redo_log = 0;
volatile unsigned long tot_pstm_time_flush_data = 0;
volatile unsigned long tot_pstm_time_tx = 0;
volatile unsigned long tot_pstm_size_flush = 0;
volatile unsigned long tot_pstm_nb_tx = 0;
volatile unsigned long tot_pstm_nb_flush = 0;

__thread unsigned long ts1, ts2, ts3, ts4;

void pstm_before_tm_start(int numThread) {
  pstm_nvm_create(numThread);
  pstm_vlog_init(numThread);
  pstm_dram_create();
  pstm_plog_init();
}

void pstm_after_thread_start(int threadID){
  pstm_vlog_init_thread(threadID);
}

void pstm_after_store(uint64_t *addr, uint64_t value){
  pstm_vlog_collect(addr, value);
}

void pstm_before_tx_start() {
  pstm_vlog_clear();
  if (ts1 == 0) {
    ts1 = rdtscp();
  }
  
}

void pstm_after_tx_commit(uint64_t ts) {
  if (ts == 0) {
    pstm_vlog_clear();
    return;
  }
  pstm_vlog_commit(ts);
  pstm_plog_collect();
  pstm_vlog_clear();
  ts4 = rdtscp();

  if (ts2 != 0) {
    pstm_time_flush_redo_log += ts3 - ts2;
    pstm_time_flush_data += ts4 - ts3;

    pstm_nb_flush ++;
  }
  pstm_time_tx += ts4 - ts1;
  pstm_nb_tx ++;

  ts1 = 0;
  ts2 = 0;
  ts3 = 0;
  ts4 = 0;
}

void pstm_before_thread_exit(){
  pstm_vlog_free();
  __sync_add_and_fetch(&tot_pstm_time_flush_redo_log, pstm_time_flush_redo_log);
  __sync_add_and_fetch(&tot_pstm_time_flush_data, pstm_time_flush_data);
  __sync_add_and_fetch(&tot_pstm_time_tx, pstm_time_tx);
  __sync_add_and_fetch(&tot_pstm_size_flush, pstm_size_flush);
  __sync_add_and_fetch(&tot_pstm_nb_tx, pstm_nb_tx);
  __sync_add_and_fetch(&tot_pstm_nb_flush, pstm_nb_flush);
}

void pstm_after_tm_exit() {
  pstm_plog_end();
  pstm_nvm_close();
  printf("nb_tx:\t\t%lu\nnb_flush:\t%lu\ntime_tx:\t%lf\ntime_log:\t%lf\ntime_data:\t%lf\nsize_flush:\t%lf\n",
    tot_pstm_nb_tx,tot_pstm_nb_flush,
    (double)tot_pstm_time_tx/(double)tot_pstm_nb_tx, 
    (double)tot_pstm_time_flush_redo_log/(double)tot_pstm_nb_flush, 
    (double)tot_pstm_time_flush_data/(double)tot_pstm_nb_flush, 
    (double)tot_pstm_size_flush/(double)tot_pstm_nb_flush);
}