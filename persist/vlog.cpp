#include "vlog.h"
#include "page.h"

__thread pstm_vlog_t *thread_vlog_entry;
FreeVlogCollecter **free_vlog_collecters;
ReadyVlogCollecter **ready_vlog_collecters;
__thread int thread_id;
__thread int flusher_id;
#ifdef PDEBUG
std::atomic<uint64_t> commit_count = 0;
#endif
int thread_count;

void pstm_vlog_init(int thread_num) {
  thread_count = thread_num;
  free_vlog_collecters = (FreeVlogCollecter **)malloc(sizeof(FreeVlogCollecter *) * thread_num);
  ready_vlog_collecters = (ReadyVlogCollecter **)malloc(sizeof(ReadyVlogCollecter *) * flusher_count);
  for (int i = 0; i < thread_num; i ++) {
    free_vlog_collecters[i] = new FreeVlogCollecter(FREE_VLOG_PER_THREAD+1);
    for (int j = 0; j < FREE_VLOG_PER_THREAD; j ++) {
      pstm_vlog_t *vlog = (pstm_vlog_t *)malloc(sizeof(pstm_vlog_t));
      vlog->buffer = (uint64_t*)malloc(VLOG_BUFEER_SIZE);
      vlog->group_dep_buffer = (uint64_t*)malloc(DEP_MAX_NUM * sizeof(uint64_t)*2);
      vlog->group_dep_count = 0;
      vlog->ts = 0;
      vlog->log_count = 0;
      vlog->dep_count = 0;
      vlog->thread_id = i;
      free_vlog_collecters[i]->put(vlog);
    }
  }
  for (int i = 0; i < flusher_count; i ++) {
    ready_vlog_collecters[i] = new ReadyVlogCollecter(READY_VLOG_PER_FLUSHER+1);
  }
}

void pstm_vlog_init_thread(int threadID) {
  // thread_id = (int)__sync_fetch_and_add(&thread_count,1);
  thread_id = threadID;
  flusher_id = thread_id % flusher_count;
  if (FLUSHER_TYPE == 0) thread_vlog_entry = free_vlog_collecters[thread_id]->get();
}

void pstm_vlog_exit_thread() {
  if (thread_vlog_entry != NULL) {
    free(thread_vlog_entry->buffer);
    free(thread_vlog_entry);
  }
}

void pstm_vlog_begin() {
  if (FLUSHER_TYPE != 0 && thread_vlog_entry == NULL) {
    thread_vlog_entry = free_vlog_collecters[thread_id]->get();
  }
  thread_vlog_entry->log_count = 0;
  thread_vlog_entry->dep_count = 0;
  thread_vlog_entry->state.store(VLOG_INVAILD);
}

// TODO: modify in log entry // done
void pstm_vlog_collect(void *addr, uint64_t value, uint64_t index) {
  uint64_t log_count = thread_vlog_entry->log_count;

  assert(log_count <= VLOG_MAX_NUM);

  if(!IS_PMEM(addr)) return;
  // if (index != UINT64_MAX) {
  //   ASSERT(thread_vlog_entry->buffer[index * 2] == (uint64_t)addr - (uint64_t)pstm_dram_ptr);
  //   thread_vlog_entry->buffer[index * 2 + 1] = value;
  // }
  else {
    thread_vlog_entry->buffer[log_count * 2] = (uint64_t)addr - (uint64_t)pstm_dram_ptr;
    thread_vlog_entry->buffer[log_count * 2 + 1] = value;
    thread_vlog_entry->log_count ++;
  }
}

void pstm_vlog_before_gen_ts() {
  // if (FLUSHER_TYPE != 0 && thread_vlog_entry->log_count != 0)
  #ifdef ORDER_COLLECT
  ready_vlog_collecters[flusher_id]->vlog_collect_lock.lock();
  #endif
}

void pstm_vlog_after_gen_ts(uint64_t ts) {
  thread_vlog_entry->ts = ts;
  thread_vlog_entry->state.store(VLOG_PRE_COMMIT);
  if (FLUSHER_TYPE != 0 && thread_vlog_entry->log_count != 0) {
    ready_vlog_collecters[flusher_id]->put(thread_vlog_entry);
    // ready_vlog_collecters[flusher_id]->vlog_collect_lock.unlock();
  }
  ready_vlog_collecters[flusher_id]->last_ready_ts = ts;
  #ifdef ORDER_COLLECT
  ready_vlog_collecters[flusher_id]->vlog_collect_lock.unlock();
  #endif
}

void pstm_vlog_abort() {
  if (FLUSHER_TYPE == 0 || thread_vlog_entry->state != VLOG_PRE_COMMIT || thread_vlog_entry->log_count == 0) {
    thread_vlog_entry->log_count = 0;
    thread_vlog_entry->dep_count = 0;
    thread_vlog_entry->state.store(VLOG_INVAILD);
  }
  else {
    thread_vlog_entry->log_count = 0;
    thread_vlog_entry->dep_count = 0;
    thread_vlog_entry->state.store(VLOG_ABORT);
    thread_vlog_entry = free_vlog_collecters[thread_id]->get();
    thread_vlog_entry->log_count = 0;
    thread_vlog_entry->dep_count = 0;
    thread_vlog_entry->state.store(VLOG_INVAILD);
  }

} 

void pstm_vlog_commit(uint64_t ts) {
  thread_vlog_entry->state.store(VLOG_COMMITTED);
  #ifdef PDEBUG
  commit_count += thread_vlog_entry->log_count;
  #endif
  if (FLUSHER_TYPE != 0 && thread_vlog_entry->log_count != 0) {
    // ready_vlog_collecters[flusher_id]->put(thread_vlog_entry);
    thread_vlog_entry = NULL;
  }
}

void pstm_vlog_free() {
  for (int i = 0; i < thread_count; i ++) {
    while (!free_vlog_collecters[i]->empty()) {
      pstm_vlog_t *vlog = free_vlog_collecters[i]->get();
      free(vlog->buffer);
      free(vlog);
    }
    delete free_vlog_collecters[i];
  }
  free(free_vlog_collecters);
}

void pstm_vlog_trace_dep(uint64_t lock_val) {
  int dep_thread_id = (lock_val >> 4) & ((1 << 5) - 1);
  if (!(dep_thread_id & 1)) return;
  dep_thread_id >>= 1;
  if (dep_thread_id == thread_id) return;
  uint64_t dep_ts = lock_val >> 9;
  if (dep_ts == 0) return;
  uint64_t dep_offset = pstm_plog_trace_dep(dep_thread_id, dep_ts);
  if (dep_offset != UINT64_MAX) {
    assert(thread_vlog_entry->group_dep_count < DEP_MAX_NUM);
    if (thread_vlog_entry->dep_count == 0) {
      thread_vlog_entry->group_dep_buffer[thread_vlog_entry->group_dep_count*2] = thread_vlog_entry->ts | (1UL<<63);
      thread_vlog_entry->group_dep_buffer[thread_vlog_entry->group_dep_count*2+1] = 0;
    }
    thread_vlog_entry->group_dep_buffer[thread_vlog_entry->group_dep_count*2] = lock_val | 0x1;
    thread_vlog_entry->group_dep_buffer[thread_vlog_entry->group_dep_count*2+1] = dep_offset;
    thread_vlog_entry->group_dep_count++;
    thread_vlog_entry->dep_count++;

    thread_vlog_entry->group_dep_buffer[(thread_vlog_entry->group_dep_count-thread_vlog_entry->dep_count)*2+1] = thread_vlog_entry->dep_count;
  }
}