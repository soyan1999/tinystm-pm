#include "vlog.h"

pstm_vlog_t *pstm_vlogs;
__thread int thread_id;
int thread_count;

void pstm_vlog_init(int thread_num) {
  thread_count = thread_num;
  pstm_vlogs = (pstm_vlog_t *)malloc(sizeof(pstm_vlog_t)*thread_num);
  for (int i = 0; i < thread_num; i ++) {
    pstm_vlogs[i].buffer = (uint64_t*)malloc(VLOG_BUFEER_SIZE);
    pstm_vlogs[i].log_count = 0;
    pstm_vlogs[i].ts;
  }
}

void pstm_vlog_init_thread(int threadID) {
  // thread_id = (int)__sync_fetch_and_add(&thread_count,1);
  thread_id = threadID;
}

void pstm_vlog_clear() {
  pstm_vlogs[thread_id].log_count = 0;
}

void pstm_vlog_collect(void *addr, uint64_t value) {
  uint64_t log_count = pstm_vlogs[thread_id].log_count;

  assert(log_count <= VLOG_MAX_NUM);

  pstm_vlogs[thread_id].buffer[log_count * 2] = (uint64_t)addr - (uint64_t)pstm_dram_ptr;
  pstm_vlogs[thread_id].buffer[log_count * 2 + 1] = value;
  log_count ++;
}

void pstm_vlog_commit(uint64_t ts) {
  pstm_vlogs[thread_id].ts = ts;
}

void pstm_vlog_free() {
  for (int i = 0; i < thread_count; i ++) {
    free(pstm_vlogs[i].buffer);;
  }
  free(pstm_vlogs);
}