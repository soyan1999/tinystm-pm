#ifndef _PMDK_H_
#define _PMDK_H_

#include <libpmemobj.h>

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

# define LAYOUT_NAME "pmdk"
# define POOL_SIZE (1 * 1024 * 1024 * 1024)
# define POOL_PATH "/mnt/pmem0/ysha/tinystm-pm/pmdk.pool"
// # define POOL_PATH "./pmdk.pool"


#ifdef __cplusplus
extern "C" {
#endif

POBJ_LAYOUT_BEGIN(pmdk);
POBJ_LAYOUT_ROOT(pmdk, struct pool_root);
POBJ_LAYOUT_TOID(pmdk, struct value);
POBJ_LAYOUT_END(pmdk);

typedef struct value {
  char v;
} value_t;

typedef struct pool_root {
  TOID(struct value) head;
} pool_root_t;


extern PMEMobjpool *pool_;


void pobj_before_tm_start();                              // init nvm pool
void pobj_before_store(void *addr, size_t len);       // add to undo log
// void pobj_before_tx_begin();                              // tx begin
// void pobj_after_tx_commit(uint64_t ts);                   // tx end
void pobj_after_tm_end();                                 // close pool

void *pobj_malloc(size_t len);
void pobj_free(void *addr);
void *pobj_tx_malloc(size_t len);
void pobj_tx_free(void *addr);

#ifdef __cplusplus
}
#endif

#endif