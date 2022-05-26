#ifndef _PLOG_H_
#define _PLOG_H_

#include "pmem.h"
#include "global.h"

#define BEGING_FLAG(ts)       (ts)
#define END_FLAG(cnt)         (cnt<<2|0x2)
#define IS_END_FLAG(flag)     (flag&0x2)

#define INC_LOG_PTR(ptr)      (ptr=(ptr+1)&(PSTM_LOG_SIZE-1))
#define FLUSHER_TYPE          0
#define TOTAL_FLUSHER_NUM     1
#define MAX_REPLAY_SIZE       10000
#define MONOTONIC             0

// #define ENABLE_REPLAY
#define ENABLE_FLUSH
#define ENABLE_VLOG
#define ENABLE_COMBINE
#define ORDER_COLLECT
// #define USE_NTSTORE
#define GROUP_SIZE UINT64_MAX //UINT64_MAX
#define TRACE_DEP

// #ifdef __cplusplus
// extern "C" {
// #endif

// extern __thread uint64_t last_persist_ts;

extern int flusher_count;

void pstm_plog_init();
void pstm_plog_commit();
// void pstm_plog_init_thread();
void pstm_plog_exit_thread();
void pstm_plog_end();
void pstm_plog_block_read(uint64_t ts);
uint64_t pstm_plog_trace_dep(int thread_id, uint64_t ts);

// #ifdef __cplusplus
// }
// #endif

#endif