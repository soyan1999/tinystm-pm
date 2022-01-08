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

void pstm_before_tm_start(int numThread);                 // init nvm,vlog and shadowdram, set crash value
void pstm_after_thread_start(int threadID);               // init thread_id
void pstm_after_store(uint64_t *addr, uint64_t value);    // collect volatile log
void pstm_after_tx_commit(uint64_t ts);                   // collect and merge logs, flush
void pstm_before_thread_exit();                           // free volatile log
void pstm_after_tm_exit();                                // free collecter nvm and shadowdram, set crash value



#ifdef __cplusplus
}
#endif

#endif