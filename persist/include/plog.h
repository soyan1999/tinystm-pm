#ifndef _PLOG_H_
#define _PLOG_H_

#include "pmem.h"
#include "global.h"

#define BEGING_FLAG(ts)       (ts)
#define END_FLAG(cnt)         (cnt<<2|0x2)
#define IS_END_FLAG(flag)     (flag&0x2)

#define INC_LOG_PTR(ptr)      (ptr=(ptr+1)&(PSTM_LOG_SIZE-1))
#define FLUSHER_TYPE          1

// #ifdef __cplusplus
// extern "C" {
// #endif

extern __thread uint64_t last_persist_ts;

void pstm_plog_init();
void pstm_plog_collect();
void pstm_plog_end();

// #ifdef __cplusplus
// }
// #endif

#endif