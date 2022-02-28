#ifndef _VLOG_H_
#define _VLOG_H_

#include <folly/MPMCQueue.h>
#include <folly/ProducerConsumerQueue.h>
#include <chrono>
#include "plog.h"
#include "pmem.h"
#include "page.h"
#include "global.h"

#define VLOG_BUFEER_SIZE (32*1024*1024)
#define VLOG_MAX_NUM (VLOG_BUFEER_SIZE/16)
#define VLOG_COLLECTER_CAPACITY 20
#define VLOG_FREE_CAPACITY 30

#define FREE_VLOG_PER_THREAD 10

// #ifdef __cplusplus
// extern "C" {
// #endif

class ReadyVlogCollecter {
  folly::MPMCQueue<pstm_vlog_t*> vlog_queue;
public:
  ReadyVlogCollecter(int capacity):vlog_queue(capacity) {}

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
  }
};


typedef struct pstm_vlog {
  uint64_t ts;
  uint64_t log_count;
  uint64_t thread_id;
  uint64_t *buffer;
} pstm_vlog_t;


extern FreeVlogCollecter **free_vlog_collecters;
extern ReadyVlogCollecter **ready_vlog_collecters;
extern __thread pstm_vlog_t *thread_vlog_entry;
extern __thread int thread_id;
extern __thread int flusher_id;
extern int thread_count;


void pstm_vlog_init(int thread_num);
void pstm_vlog_init_thread(int threadID);
void pstm_vlog_exit_thread();
void pstm_vlog_begin();
void pstm_vlog_collect(void *addr, uint64_t value);
void pstm_vlog_commit(uint64_t ts);
void pstm_vlog_free();

// #ifdef __cplusplus
// }
// #endif

#endif