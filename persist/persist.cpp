#include "persist.h"
#include "global.h"
#include "pmem.h"
#include "page.h"
#include "plog.h"
#include "vlog.h"
#include "rdtsc.h"


#include "time_measure.h"

__thread unsigned long pstm_time_flush_redo_log = 0;
__thread unsigned long pstm_time_flush_data = 0;
__thread unsigned long pstm_time_tx = 0;
__thread unsigned long pstm_size_flush = 0;
__thread unsigned long pstm_nb_tx = 0;
__thread unsigned long pstm_nb_atx = 0;
__thread unsigned long pstm_nb_flush = 0;
__thread unsigned long pstm_nb_step = 0;
__thread unsigned long pstm_nb_group = 0;
__thread unsigned long pstm_nb_force_flush = 0;
__thread unsigned long pstm_nb_dep_trace = 0;
__thread unsigned long pstm_nb_tx_begin = 0;
__thread unsigned long pstm_nb_tx_abort = 0;
volatile unsigned long tot_pstm_time_flush_redo_log = 0;
volatile unsigned long tot_pstm_time_flush_data = 0;
volatile unsigned long tot_pstm_time_tx = 0;
volatile unsigned long tot_pstm_size_flush = 0;
volatile unsigned long tot_pstm_nb_tx = 0;
volatile unsigned long tot_pstm_nb_atx = 0;
volatile unsigned long tot_pstm_nb_flush = 0;
volatile unsigned long tot_pstm_nb_step = 0;
volatile unsigned long tot_pstm_nb_group = 0;
volatile unsigned long tot_pstm_nb_force_flush = 0;
volatile unsigned long tot_pstm_nb_dep_trace = 0;
volatile unsigned long tot_pstm_nb_tx_begin = 0;
volatile unsigned long tot_pstm_nb_tx_abort = 0;

__thread unsigned long ts1, ts2, ts3, ts4, ts5, ts6;

void pstm_before_tm_start(int numThread) {
  pstm_nvm_create(numThread);
  pstm_vlog_init(numThread);
  #ifdef TIME_MEASURE
  time_measure_init(numThread);
  #endif
  pstm_dram_create();
  pstm_plog_init();
}

void pstm_after_thread_start(int threadID){
  pstm_vlog_init_thread(threadID);

  if (pstm_time_flush_redo_log != 0) {
    __sync_add_and_fetch(&tot_pstm_time_flush_redo_log, pstm_time_flush_redo_log);
    pstm_time_flush_redo_log = 0;
  }
  if (pstm_time_flush_data != 0) {
    __sync_add_and_fetch(&tot_pstm_time_flush_data, pstm_time_flush_data);
    pstm_time_flush_data = 0;
  }
  if (pstm_time_tx != 0) {
    __sync_add_and_fetch(&tot_pstm_time_tx, pstm_time_tx);
    pstm_time_tx = 0;
  }
  if (pstm_size_flush != 0) {
    __sync_add_and_fetch(&tot_pstm_size_flush, pstm_size_flush);
    pstm_size_flush = 0;
  }
  if (pstm_nb_tx != 0) {
    __sync_add_and_fetch(&tot_pstm_nb_tx, pstm_nb_tx);
    pstm_nb_tx = 0;
  }
  if (pstm_nb_atx != 0) {
    __sync_add_and_fetch(&tot_pstm_nb_atx, pstm_nb_atx);
    pstm_nb_atx = 0;
  }
  if (pstm_nb_flush != 0) {
    __sync_add_and_fetch(&tot_pstm_nb_flush, pstm_nb_flush);
    pstm_nb_flush = 0;
  }
  if (pstm_nb_step != 0) {
    __sync_add_and_fetch(&tot_pstm_nb_step, pstm_nb_step);
    pstm_nb_step = 0;
  }
  if (pstm_nb_group != 0) {
    __sync_add_and_fetch(&tot_pstm_nb_group, pstm_nb_group);
    pstm_nb_group = 0;
  }
  if (pstm_nb_force_flush != 0) {
    __sync_add_and_fetch(&tot_pstm_nb_force_flush, pstm_nb_force_flush);
    pstm_nb_force_flush = 0;
  }
  if (pstm_nb_tx_begin != 0) {
    __sync_add_and_fetch(&tot_pstm_nb_tx_begin, pstm_nb_tx_begin);
    pstm_nb_tx_begin = 0;
  }
  if (pstm_nb_tx_abort != 0) {
    __sync_add_and_fetch(&tot_pstm_nb_tx_abort, pstm_nb_tx_abort);
    pstm_nb_tx_abort = 0;
  }


  #ifdef TIME_MEASURE
  time_measure_enter_thread(threadID);
  #endif
  // pstm_plog_init_thread();
}

void pstm_after_store(uint64_t *addr, uint64_t value, uint64_t index){
  #ifdef ENABLE_VLOG
  pstm_vlog_collect(addr, value, index);
  #endif
}

void pstm_after_read_unlock(uint64_t *addr, uint64_t modify_ts) {
  if (IS_PMEM(addr) && MONOTONIC == 1) {
    pstm_plog_block_read(modify_ts);
  }
}

void pstm_before_tx_start() {
  pstm_vlog_begin();
  pstm_plog_begin();
  pstm_nb_tx_begin ++;
  // if (ts1 == 0) {
  //   ts1 = rdtscp();
  // }
  
}

void pstm_before_gen_ts() {
  pstm_vlog_before_gen_ts();
}

void pstm_after_gen_ts(uint64_t ts) {
  pstm_vlog_after_gen_ts(ts);
}

void pstm_before_tx_commit(uint64_t ts) {
  // if (ts == 0) {
  //   ts1 = 0;
  //   return;
  // }
  // if (ts == 1) {
  //   return;
  // }
  pstm_nb_atx ++;
  #ifdef ENABLE_VLOG
  pstm_vlog_commit(ts);
  #endif
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
  pstm_vlog_abort();
  pstm_plog_abort();
  pstm_nb_tx_abort ++;
}

void pstm_before_thread_exit(){
  pstm_vlog_exit_thread();
  pstm_plog_exit_thread();

  if (pstm_time_flush_redo_log != 0) {
    __sync_add_and_fetch(&tot_pstm_time_flush_redo_log, pstm_time_flush_redo_log);
    pstm_time_flush_redo_log = 0;
  }
  if (pstm_time_flush_data != 0) {
    __sync_add_and_fetch(&tot_pstm_time_flush_data, pstm_time_flush_data);
    pstm_time_flush_data = 0;
  }
  if (pstm_time_tx != 0) {
    __sync_add_and_fetch(&tot_pstm_time_tx, pstm_time_tx);
    pstm_time_tx = 0;
  }
  if (pstm_size_flush != 0) {
    __sync_add_and_fetch(&tot_pstm_size_flush, pstm_size_flush);
    pstm_size_flush = 0;
  }
  if (pstm_nb_tx != 0) {
    __sync_add_and_fetch(&tot_pstm_nb_tx, pstm_nb_tx);
    pstm_nb_tx = 0;
  }
  if (pstm_nb_atx != 0) {
    __sync_add_and_fetch(&tot_pstm_nb_atx, pstm_nb_atx);
    pstm_nb_atx = 0;
  }
  if (pstm_nb_flush != 0) {
    __sync_add_and_fetch(&tot_pstm_nb_flush, pstm_nb_flush);
    pstm_nb_flush = 0;
  }
  if (pstm_nb_step != 0) {
    __sync_add_and_fetch(&tot_pstm_nb_step, pstm_nb_step);
    pstm_nb_step = 0;
  }
  if (pstm_nb_group != 0) {
    __sync_add_and_fetch(&tot_pstm_nb_group, pstm_nb_group);
    pstm_nb_group = 0;
  }
  if (pstm_nb_force_flush != 0) {
    __sync_add_and_fetch(&tot_pstm_nb_force_flush, pstm_nb_force_flush);
    pstm_nb_force_flush = 0;
  }
  if (pstm_nb_dep_trace != 0) {
    __sync_add_and_fetch(&tot_pstm_nb_dep_trace, pstm_nb_dep_trace);
    pstm_nb_dep_trace = 0;
  }
  if (pstm_nb_tx_begin != 0) {
    __sync_add_and_fetch(&tot_pstm_nb_tx_begin, pstm_nb_tx_begin);
    pstm_nb_tx_begin = 0;
  }
  if (pstm_nb_tx_abort != 0) {
    __sync_add_and_fetch(&tot_pstm_nb_tx_abort, pstm_nb_tx_abort);
    pstm_nb_tx_abort = 0;
  }
  // __sync_add_and_fetch(&tot_pstm_time_flush_redo_log, pstm_time_flush_redo_log);
  // __sync_add_and_fetch(&tot_pstm_time_flush_data, pstm_time_flush_data);
  // __sync_add_and_fetch(&tot_pstm_time_tx, pstm_time_tx);
  // __sync_add_and_fetch(&tot_pstm_size_flush, pstm_size_flush);
  // __sync_add_and_fetch(&tot_pstm_nb_tx, pstm_nb_tx);
  // __sync_add_and_fetch(&tot_pstm_nb_atx, pstm_nb_atx);
  // __sync_add_and_fetch(&tot_pstm_nb_flush, pstm_nb_flush);
  // __sync_add_and_fetch(&tot_pstm_nb_step, pstm_nb_step);
  // __sync_add_and_fetch(&tot_pstm_nb_group, pstm_nb_group);
  // __sync_add_and_fetch(&tot_pstm_nb_force_flush, pstm_nb_force_flush);
  // __sync_add_and_fetch(&tot_pstm_nb_dep_trace, pstm_nb_dep_trace);
}

void pstm_after_tm_exit() {
  pstm_plog_end();
  pstm_vlog_free();
  pstm_nvm_check();
  pstm_nvm_close();
  pstm_dram_close();

  #ifdef TIME_MEASURE
  #ifdef TRACE_DEP
  time_measure_lost_deptrc();
  #else
  time_measure_lost_group_commit();
  #endif
  #endif

  #ifdef DROP_CNT
  #ifdef TRACE_DEP
  time_measure_delay_deptrc();
  #else
  time_measure_delay_group();
  #endif
  #endif
  printf("nb_abort:\t%lu\nnb_begin:\t%lu\ntot_pstm_nb_dep_trace:\t%lu\nnb_force_flush:\t%lu\nnb_atx:\t\t%lu\nnb_tx:\t\t%lu\ntot_log_size:\t%lu\navg_log_size:\t%lf\navg_group_size:\t%lf\n",
      tot_pstm_nb_tx_abort,tot_pstm_nb_tx_begin,tot_pstm_nb_dep_trace,tot_pstm_nb_flush,tot_pstm_nb_atx, tot_pstm_nb_tx, tot_pstm_size_flush,
      (double)tot_pstm_size_flush/(double)tot_pstm_nb_tx,
      tot_pstm_nb_step?((double)tot_pstm_nb_group/(double)tot_pstm_nb_step):0);


  // printf("nb_tx:\t\t%lu\nnb_flush:\t%lu\ntime_tx:\t%lf\ntime_log:\t%lf\ntime_data:\t%lf\nsize_flush:\t%lf\nforce_flush:\t%lf\n",
  //   tot_pstm_nb_tx,tot_pstm_nb_flush,
  //   (double)tot_pstm_time_tx/(double)tot_pstm_nb_tx, 
  //   (double)tot_pstm_time_flush_redo_log/(double)tot_pstm_nb_flush, 
  //   (double)tot_pstm_time_flush_data/(double)tot_pstm_nb_flush, 
  //   (double)tot_pstm_size_flush/(double)tot_pstm_nb_flush,
  //   (double)tot_pstm_nb_force_flush/(double)tot_pstm_nb_flush);
}

int get_thread_id() {
  return thread_id;
}

void pstm_trace_dep(uint64_t lock_val) {
  #ifdef TRACE_DEP
  pstm_vlog_trace_dep(lock_val);
  #endif
}

int is_pmem(uint64_t addr) {
  return IS_PMEM(addr);
}