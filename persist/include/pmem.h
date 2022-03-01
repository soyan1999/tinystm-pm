#ifndef _PMEM_H_
#define _PMEM_H_

#include <assert.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

#include <pthread.h>

#include "global.h"
// #include "vlog.h"

#define FLUSH_X86_INST       "clwb"
// #define FLUSH_X86_INST    "clflushopt"
// #define FLUSH_X86_INST    "clflush"

#define FENCE_X86_INST       "sfence"
#define ARCH_CACHE_LINE_SIZE 64
#define PAGE_SIZE            4096

#define FLUSH_CL(_addr) \
  __asm__ volatile(FLUSH_X86_INST " (%0)" : : "r"((void*)((uint64_t)(_addr) & -ARCH_CACHE_LINE_SIZE)) : "memory") \
//

#define FENCE_PREV_FLUSHES() \
  __asm__ volatile(FENCE_X86_INST : : : "memory"); \
//

// allow circular buffer
#define FLUSH_RANGE(addr1, addr2, beginAddr, endAddr) \
  if (addr2 < addr1) { \
    for (uint64_t _addr = ((uint64_t)(addr1) & (uint64_t)-ARCH_CACHE_LINE_SIZE); \
                  _addr < (uint64_t)(endAddr); \
                  _addr += ARCH_CACHE_LINE_SIZE) { \
      __asm__ volatile(FLUSH_X86_INST " (%0)" : : "r"(((void*)_addr)) : "memory"); \
    } \
    for (uint64_t _addr = ((uint64_t)(beginAddr) & (uint64_t)-ARCH_CACHE_LINE_SIZE); \
                  _addr < (uint64_t)(addr2); \
                  _addr += ARCH_CACHE_LINE_SIZE) { \
      __asm__ volatile(FLUSH_X86_INST " (%0)" : : "r"(((void*)_addr)) : "memory"); \
    } \
  } else { \
    for (uint64_t _addr = ((uint64_t)(addr1) & (uint64_t)-ARCH_CACHE_LINE_SIZE); \
                  _addr < (uint64_t)(addr2); \
                  _addr += ARCH_CACHE_LINE_SIZE) { \
      __asm__ volatile(FLUSH_X86_INST " (%0)" : : "r"(((void*)_addr)) : "memory"); \
    } \
  } \

// not allow circular
#define FLUSH_BLOCK(addr1, addr2) \
  for (uint64_t _addr = ((uint64_t)(addr1) & (uint64_t)-ARCH_CACHE_LINE_SIZE); \
                _addr < (uint64_t)(addr2); \
                _addr += ARCH_CACHE_LINE_SIZE) { \
    __asm__ volatile(FLUSH_X86_INST " (%0)" : : "r"(((void*)_addr)) : "memory"); \
  } \


#define PSTM_HEAP_SIZE_PER_THREAD (4UL*1024*1024*1024)
#define PSTM_SHARE_HEAP_SIZE (8UL*1024*1024*1024)
#define PSTM_LOG_SIZE (4UL*1024*1024*1024)


#ifdef __cplusplus
extern "C" {
#endif


extern void *pstm_nvram_ptr;
extern void *pstm_nvram_heap_ptr;
extern void *pstm_nvram_logs_ptr;
extern void *pstm_nvram_logs_root_ptr;
extern void *pstm_nvram_priv_heap_ptr;

extern __thread void *pstm_nvram_pri_ptr;

typedef struct log_root {
  uint64_t crash;
  uint64_t log_start_off;
  uint64_t log_end_off;
  uint64_t padding[509];
} __attribute__((packed)) log_root_t;

void pstm_nvm_create(int numThread);
void pstm_nvm_close();
void *pstm_nvmalloc(long);
void *pstm_local_nvmalloc(int, long);

#ifdef __cplusplus
}
#endif
#endif