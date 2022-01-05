#ifndef _GLOBAL_H_
#define _GLOBAL_H_


#include <stdint.h>


#define ARCH_CACHE_LINE_SIZE    64




#ifdef __cplusplus
extern "C" {
#endif

typedef uint64_t nv_ptr;

typedef struct pool_root {
  nv_ptr root_obj;
  uint64_t log_start_off;
  uint64_t log_end_off;
  uint64_t padding[5];
} __attribute__((aligned(ARCH_CACHE_LINE_SIZE))) pool_root;

typedef union cache_line_ {
  struct {
    volatile uint64_t ts;
    volatile uint64_t LLC;
    volatile uint64_t LPC;
    volatile uint64_t version;
    volatile uint64_t isInTX;
    volatile uint64_t padding[3];
  } __attribute__((packed)) comm;
  struct {
    volatile int32_t epoch_next;
    volatile int32_t write_log_next;
    volatile int32_t flush_epoch;
    volatile int32_t write_log_start;
    volatile uint64_t padding[6];
  } __attribute__((packed)) log_ptrs;
  struct {
    int32_t isExit;
    int32_t nbThreads;
    int32_t nbReplayers;
    int32_t allocEpochs;
    int32_t allocLogSize;
    int32_t localMallocSize;
    int32_t spinsFlush;
    int32_t epochTimeout;
  } __attribute__((packed)) info;
  volatile uint64_t ts; /* padded TS */
  volatile uint64_t padding[8]; /* >64B, TODO: lots of aborts using 8 */
} __attribute__((aligned(ARCH_CACHE_LINE_SIZE))) cache_line_s;

typedef union large_cache_line_ {
  struct {
    volatile uint64_t ts;
    volatile uint64_t LLC;
    volatile uint64_t LPC;
    volatile uint64_t version;
    volatile uint64_t isInTX;
    volatile uint64_t padding[3];
  } __attribute__((packed)) comm;
  struct {
    volatile uint64_t ts;
    volatile uint64_t prevTS;
    volatile uint64_t logPos;
    volatile uint64_t prevLogPos;
    volatile uint64_t flushedMarker;
    volatile uint64_t isUpdate;
    volatile uint64_t padding[2];
  } __attribute__((packed)) pcwm;
  struct {
    volatile uint64_t ts;
    volatile uint64_t isInTX;
    volatile uint64_t isReturnToApp;
    volatile uint64_t globalMarkerTS;
    volatile uint64_t globalMarkerIntent;
    volatile uint64_t isFlushed;
    volatile uint64_t waitSnapshot;
    volatile uint64_t padding[1];
  } __attribute__((packed)) comm2;
  volatile uint64_t ts; /* padded TS */
  volatile uint64_t padding[256];
} __attribute__((aligned(32*ARCH_CACHE_LINE_SIZE))) large_cache_line_s;


#ifdef __cplusplus
}
#endif


#endif