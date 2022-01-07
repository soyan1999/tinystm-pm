#include "persist.h"

inline void pstm_before_tm_start(int numThread) {
  pstm_nvm_create(numThread);
  pstm_plog_init();
}

inline void pstm_after_thread_start(){
  pstm_plog_init();
}

inline void pstm_after_store(uint64_t *addr, uint64_t value){
  pstm_vlog_collect(addr, value);
}

inline void pstm_after_tx_commit(uint64_t ts) {
  pstm_plog_collect();
}

inline void pstm_before_thread_exit(){
  pstm_vlog_free();
}

inline void pstm_after_tm_exit() {
  pstm_plog_end();
  pstm_nvm_close();
}