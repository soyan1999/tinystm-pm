#include "persist.h"
#include "global.h"
#include "pmem.h"
#include "page.h"
#include "plog.h"
#include "vlog.h"
#include "rdtsc.h"

__thread unsigned long pstm_time_flush_redo_log = 0;
__thread unsigned long pstm_time_flush_data = 0;
__thread unsigned long pstm_time_tx = 0;
__thread unsigned long pstm_size_flush = 0;
__thread unsigned long pstm_nb_tx = 0;
__thread unsigned long pstm_nb_flush = 0;
__thread unsigned long pstm_is_force_flush = 0;
__thread unsigned long pstm_nb_force_flush = 0;
volatile unsigned long tot_pstm_time_flush_redo_log = 0;
volatile unsigned long tot_pstm_time_flush_data = 0;
volatile unsigned long tot_pstm_time_tx = 0;
volatile unsigned long tot_pstm_size_flush = 0;
volatile unsigned long tot_pstm_nb_tx = 0;
volatile unsigned long tot_pstm_nb_flush = 0;
volatile unsigned long tot_pstm_nb_force_flush = 0;

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

void pstm_after_store(uint64_t *addr, uint64_t value, uint64_t index){
  pstm_vlog_collect(addr, value, index);
}

void pstm_after_read_unlock(uint64_t *addr, uint64_t modify_ts) {
  if (IS_PMEM(addr)) {
    pstm_plog_block_read(modify_ts);
  }
}

void pstm_before_tx_start() {
  pstm_vlog_begin();
  // if (ts1 == 0) {
  //   ts1 = rdtscp();
  // }
  
}

void pstm_before_tx_commit(uint64_t ts) {
  // if (ts == 0) {
  //   ts1 = 0;
  //   return;
  // }
  // if (ts == 1) {
  //   return;
  // }
  pstm_vlog_commit(ts);
  pstm_plog_commit();
  // pstm_vlog_clear();
  // ts4 = rdtscp();

  // if (ts2 != 0) {
  //   pstm_time_flush_redo_log += ts3 - ts2;
  //   pstm_time_flush_data += ts4 - ts3;

  //   pstm_nb_flush ++;
  // }
  // pstm_time_tx += ts4 - ts1;
  // pstm_nb_force_flush += pstm_is_force_flush;
  // pstm_is_force_flush = 0;
  // pstm_nb_tx ++;

  // ts1 = 0;
  // ts2 = 0;
  // ts3 = 0;
  // ts4 = 0;
}

void pstm_before_tx_abort() {
  // ts1 = 0;
  // ts2 = 0;
  // ts3 = 0;
  // ts4 = 0;
  // pstm_vlog_clear();
}

void pstm_before_thread_exit(){
  pstm_vlog_exit_thread();
  pstm_plog_exit_thread();
  __sync_add_and_fetch(&tot_pstm_time_flush_redo_log, pstm_time_flush_redo_log);
  __sync_add_and_fetch(&tot_pstm_time_flush_data, pstm_time_flush_data);
  __sync_add_and_fetch(&tot_pstm_time_tx, pstm_time_tx);
  __sync_add_and_fetch(&tot_pstm_size_flush, pstm_size_flush);
  __sync_add_and_fetch(&tot_pstm_nb_tx, pstm_nb_tx);
  __sync_add_and_fetch(&tot_pstm_nb_flush, pstm_nb_flush);
  __sync_add_and_fetch(&tot_pstm_nb_force_flush, pstm_nb_force_flush);
}

void pstm_after_tm_exit() {
  pstm_plog_end();
  pstm_vlog_free();
  pstm_nvm_close();
  printf("nb_tx:\t\t%lu\nnb_flush:\t%lu\ntime_tx:\t%lf\ntime_log:\t%lf\ntime_data:\t%lf\nsize_flush:\t%lf\nforce_flush:\t%lf\n",
    tot_pstm_nb_tx,tot_pstm_nb_flush,
    (double)tot_pstm_time_tx/(double)tot_pstm_nb_tx, 
    (double)tot_pstm_time_flush_redo_log/(double)tot_pstm_nb_flush, 
    (double)tot_pstm_time_flush_data/(double)tot_pstm_nb_flush, 
    (double)tot_pstm_size_flush/(double)tot_pstm_nb_flush,
    (double)tot_pstm_nb_force_flush/(double)tot_pstm_nb_flush);
}