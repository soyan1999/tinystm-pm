#ifndef _PERSIST_H_
#define _PERSIST_H_

// #include "global.h"
#include "pmem.h"
#include "page.h"
// #include "plog.h"
// #include "vlog.h"
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif

extern __thread unsigned long ts1, ts2, ts3, ts4;
extern __thread unsigned long pstm_nb_tx;
extern __thread unsigned long pstm_nb_flush;
extern __thread unsigned long pstm_size_flush;

void pstm_before_tm_start(int numThread);                 // init nvm,vlog and shadowdram, set crash value
void pstm_after_thread_start(int threadID);               // init thread_id
void pstm_after_store(uint64_t *addr, uint64_t value, uint64_t index);    // collect volatile log
void pstm_after_read_unlock(uint64_t *addr, uint64_t modify_ts);          // check read persist
void pstm_before_tx_start();                              // collect ts
void pstm_before_gen_ts();                                // set log vaild and lock
void pstm_after_gen_ts(uint64_t ts);                      // insert to queue and unlock
void pstm_before_tx_commit(uint64_t ts);                   // collect and merge logs, flush, before release lock
void pstm_before_tx_abort();                               // clean vlog
void pstm_before_thread_exit();                           // free volatile log
void pstm_after_tm_exit();                                // free collecter nvm and shadowdram, set crash value
int get_thread_id();
void pstm_trace_dep(uint64_t lock_val);
int is_pmem(uint64_t addr);



#ifdef __cplusplus
}
#endif

#endif