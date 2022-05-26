#ifndef _VLOG_H_
#define _VLOG_H_

#include <folly/MPMCQueue.h>
#include <folly/ProducerConsumerQueue.h>
// #include <folly/SpinLock.h>
#include <mutex>
#include <chrono>
#include <atomic>
#include "plog.h"
#include "pmem.h"
#include "page.h"
#include "global.h"

#define VLOG_BUFEER_SIZE (32*1024*1024)
#define VLOG_MAX_NUM (VLOG_BUFEER_SIZE/16)
#define VLOG_COLLECTER_CAPACITY 20
#define VLOG_FREE_CAPACITY 30

#define FREE_VLOG_PER_THREAD 10
#define READY_VLOG_PER_FLUSHER 100

#define VLOG_INVAILD    0
#define VLOG_ABORT      1
#define VLOG_PRE_COMMIT 2
#define VLOG_COMMITTED  3
#define VLOG_PERSISTED  4

// #ifdef __cplusplus
// extern "C" {
// #endif


typedef struct pstm_vlog {
  uint64_t ts;
  uint64_t log_count;
  uint64_t thread_id;
  std::atomic_uint64_t state;
  uint64_t *buffer;
} pstm_vlog_t;

class ReadyVlogCollecter {
  folly::MPMCQueue<pstm_vlog_t*> vlog_queue;
public:
  ReadyVlogCollecter(int capacity):vlog_queue(capacity) {}

  // insert lock
  std::mutex vlog_collect_lock;
  uint64_t last_ready_ts;

  void put(pstm_vlog_t *vlog) {
    vlog_queue.blockingWrite(vlog);
  }

  pstm_vlog_t* get(int check_duration) {
    pstm_vlog_t* vlog;
    bool ret = vlog_queue.tryReadUntil(std::chrono::steady_clock::now()+std::chrono::steady_clock::duration(check_duration),vlog);
    return ret?vlog:nullptr;
  }

  bool empty() {
    return vlog_queue.isEmpty();
  }
};

class FreeVlogCollecter {
  folly::ProducerConsumerQueue<pstm_vlog_t*> vlog_pool;
public:
  FreeVlogCollecter(int capacity):vlog_pool(capacity) {}
  
  void put(pstm_vlog_t *vlog) {
    bool succ = false;
    while (!succ) {
      succ = vlog_pool.write(vlog);
    }
  }

  pstm_vlog_t* get() {
    pstm_vlog_t *ret;
    bool succ = false;
    while (!succ) {
      succ = vlog_pool.read(ret);
    }
    return ret;
  }

  bool empty() {
    return vlog_pool.isEmpty();
  }
};




extern FreeVlogCollecter **free_vlog_collecters;
extern ReadyVlogCollecter **ready_vlog_collecters;
extern __thread pstm_vlog_t *thread_vlog_entry;
extern __thread int thread_id;
extern __thread int flusher_id;
#ifdef PDEBUG
extern std::atomic<uint64_t> commit_count;
#endif
extern int thread_count;


void pstm_vlog_init(int thread_num);
void pstm_vlog_init_thread(int threadID);
void pstm_vlog_exit_thread();
void pstm_vlog_begin();
void pstm_vlog_collect(void *addr, uint64_t value, uint64_t index);
void pstm_vlog_before_gen_ts();
void pstm_vlog_after_gen_ts(uint64_t ts);
void pstm_vlog_commit(uint64_t ts);
void pstm_vlog_abort();
void pstm_vlog_free();
void pstm_vlog_trace_dep(uint64_t lock_val);

// #ifdef __cplusplus
// }
// #endif

#endif