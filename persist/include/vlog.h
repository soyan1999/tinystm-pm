#ifndef _VLOG_H_
#define _VLOG_H_

#include "pmem.h"
#include "page.h"
#include "global.h"

#define VLOG_BUFEER_SIZE (32*1024*1024)
#define VLOG_MAX_NUM (VLOG_BUFEER_SIZE/16)

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pstm_vlog {
  uint64_t *buffer;
  uint64_t log_count;
  uint64_t ts;
} pstm_vlog_t;

extern pstm_vlog_t *pstm_vlogs;
extern __thread int thread_id;
extern int thread_count;


void pstm_vlog_init(int thread_num);
void pstm_vlog_init_thread();
void pstm_vlog_clear();
void pstm_vlog_collect(void *addr, uint64_t value);
void pstm_vlog_commit(uint64_t ts);


#ifdef __cplusplus
}
#endif

#endif