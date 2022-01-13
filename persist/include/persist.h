#ifndef _PERSIST_H_
#define _PERSIST_H_

#include "global.h"
#include "pmem.h"
#include "page.h"
#include "plog.h"
#include "vlog.h"


#ifdef __cplusplus
extern "C" {
#endif

extern __thread unsigned long ts1, ts2, ts3, ts4;
extern __thread unsigned long pstm_nb_tx;
extern __thread unsigned long pstm_nb_flush;
extern __thread unsigned long pstm_size_flush;

void pstm_before_tm_start(int numThread);                 // init nvm,vlog and shadowdram, set crash value
void pstm_after_thread_start(int threadID);               // init thread_id
void pstm_after_store(uint64_t *addr, uint64_t value);    // collect volatile log
void pstm_before_tx_start();                              // collect ts
void pstm_after_tx_commit(uint64_t ts);                   // collect and merge logs, flush
void pstm_before_thread_exit();                           // free volatile log
void pstm_after_tm_exit();                                // free collecter nvm and shadowdram, set crash value



#ifdef __cplusplus
}
#endif

#endif