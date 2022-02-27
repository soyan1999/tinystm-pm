#include "vlog.h"
#include "page.h"

pstm_vlog_t **pstm_vlogs;
FreeVlogCollecter **free_vlog_collecters;
ReadyVlogCollecter **ready_vlog_collecters;
__thread int thread_id;
int thread_count;

void pstm_vlog_init(int thread_num) {
  thread_count = thread_num;
  free_vlog_collecters = (FreeVlogCollecter **)malloc(sizeof(FreeVlogCollecter *) * thread_num);
  for (int i = 0; i < thread_num; i ++) {
    free_vlog_collecters[i] = new FreeVlogCollecter(FREE_VLOG_PER_THREAD);
    for (int j = 0; j < FREE_VLOG_PER_THREAD; j ++) {
      pstm_vlog_t *vlog = (pstm_vlog_t *)malloc(sizeof(pstm_vlog_t));
      vlog->buffer = (uint64_t*)malloc(VLOG_BUFEER_SIZE);
      vlog->ts = 0;
      vlog->log_count = 0;
      free_vlog_collecters[i]->put(vlog);
    }
  }
}

void pstm_vlog_init_thread(int threadID) {
  // thread_id = (int)__sync_fetch_and_add(&thread_count,1);
  thread_id = threadID;
}

void pstm_vlog_clear() {
  pstm_vlogs[thread_id]->log_count = 0;
}

// TODO: modify in log entry
void pstm_vlog_collect(void *addr, uint64_t value) {
  uint64_t log_count = pstm_vlogs[thread_id]->log_count;

  assert(log_count <= VLOG_MAX_NUM);

  if(!IS_PMEM(addr)) return;
  pstm_vlogs[thread_id]->buffer[log_count * 2] = (uint64_t)addr - (uint64_t)pstm_dram_ptr;
  pstm_vlogs[thread_id]->buffer[log_count * 2 + 1] = value;
  pstm_vlogs[thread_id]->log_count ++;
}

void pstm_vlog_commit(uint64_t ts) {
  pstm_vlogs[thread_id]->ts = ts;
}

void pstm_vlog_free() {
  for (int i = 0; i < thread_count; i ++) {
    for (int j = 0; j < FREE_VLOG_PER_THREAD; j ++) {
      pstm_vlog_t *vlog = free_vlog_collecters[i]->get();
      free(vlog->buffer);
      free(vlog);
    }
    delete free_vlog_collecters[i];
  }
  free(free_vlog_collecters);
}