#include "prod-cons.h"
#include "threading.h"

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <iostream>
#include <thread>

#include "util.h"

typedef struct thr_info_ {
  int tid;
  int nbThreads;
  void *args;
  threading_callback fn;
} thr_info_s;

static pthread_t *async_thread = nullptr, *threads = nullptr;
static int nbAsyncThrs = 0, allocedAsyncThrs = 0;
static thr_info_s *info_thrs = nullptr;
static int nbThrsExec = 0, isStarted = 0;
static thread_local threading_id myThreadID;
static thread_local threading_id myCoreID = -1;

static void *thrClbk(void *args);

int prod_cons_start_thread()
{
  intptr_t pdIdx = prod_cons_init_main_thread();
  if (pdIdx != -1) {
    if (async_thread == NULL) {
      malloc_or_die(async_thread, 1);
      allocedAsyncThrs = 1;
    } else if (allocedAsyncThrs < nbAsyncThrs) {
      realloc_or_die(async_thread, allocedAsyncThrs+1);
      allocedAsyncThrs++;
    }
    thread_create_or_die(&(async_thread[nbAsyncThrs]), NULL,
      prod_cons_main_thread, (void*)pdIdx);
    nbAsyncThrs++; // not thread-safe
  }
  return pdIdx;
}

int prod_cons_join_threads()
{
  prod_cons_stop_all_threads();
  for (int i = 0; i < nbAsyncThrs; ++i) {
    thread_join_or_die(async_thread[i], NULL);
  }
  prod_cons_destroy_all_threads();
  nbAsyncThrs = 0;
  return 0;
}

void threading_start(int nbThreads, int asyncs, threading_callback callback, void *args)
{
  if (isStarted) return;
  isStarted = 1;
  nbThrsExec = nbThreads;
  // malloc_or_die(async_thread, asyncs); // done in prod_cons_start_thread()
  malloc_or_die(threads, nbThreads);
  malloc_or_die(info_thrs, nbThreads);

  for (int i = 0; i < nbThreads; ++i) {
    info_thrs[i].tid = i;
    info_thrs[i].nbThreads = nbThreads;
    info_thrs[i].args = args;
    info_thrs[i].fn = callback;
    thread_create_or_die(&(threads[i]), NULL, thrClbk, &(info_thrs[i]));
  }

  for (int i = 0; i < asyncs; ++i) {
    int pcIdx = prod_cons_start_thread();
    if (pcIdx == -1) {
      fprintf(stderr, "Could not launch all the requested asyncs\n");
      exit(EXIT_FAILURE);
    }
  }
}

void threading_join()
{
  for (int i = 0; i < nbThrsExec; ++i) {
    thread_join_or_die(threads[i], NULL);
  }
  prod_cons_join_threads();
  free(threads);
  free(info_thrs);
  free(async_thread);
  async_thread = nullptr;
  allocedAsyncThrs = 0;
  isStarted = 0;
}

int threading_async(int idAsync, prod_cons_req_fn fn, void *args)
{
  prod_cons_async_request((prod_cons_async_req_s){
    .args = args,
    .fn = fn
  }, idAsync);
  return 0;
}

int threading_getMaximumHardwareThreads()
{
  return std::thread::hardware_concurrency(); // C++11
}

void threading_pinThisThread(int coreID)
{
  myCoreID = coreID;
#ifdef __linux__
  cpu_set_t cpu_set;
  CPU_ZERO(&cpu_set);
  CPU_SET(coreID, &cpu_set);
  sched_setaffinity(0, sizeof(cpu_set_t), &cpu_set);
#endif /* __linux__ */
}

threading_id threading_getCoreID(int coreID)
{
  return myCoreID;
}

int threading_getNbThreads()
{
  return nbThrsExec;
}

threading_id threading_getMyThreadID()
{
  return myThreadID;
}

static void *thrClbk(void *args)
{
  thr_info_s *info = (thr_info_s*)args;
  myThreadID = info->tid;
  info->fn(info->tid, info->nbThreads, info->args);
  return NULL;
}
