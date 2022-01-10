#include "persist.h"

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

void pstm_after_tx_commit(uint64_t ts) {
  pstm_vlog_commit(ts);
  pstm_plog_collect();
  pstm_vlog_clear();
}

void pstm_before_thread_exit(){
  pstm_vlog_free();
}

void pstm_after_tm_exit() {
  pstm_plog_end();
  pstm_nvm_close();
}