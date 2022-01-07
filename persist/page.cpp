#include "page.h"

void *pstm_dram_ptr = NULL;

void pstm_dram_create() {
  pstm_dram_ptr = mmap(NULL , PSTM_SHARE_HEAP_SIZE + PSTM_HEAP_SIZE_PER_THREAD * THREAD_NUM, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  assert(pstm_dram_ptr != MAP_FAILED);
}

void pstm_dram_close() {
  int ret = munmap(pstm_dram_ptr, PSTM_SHARE_HEAP_SIZE + PSTM_HEAP_SIZE_PER_THREAD * THREAD_NUM);
  assert(ret == 0);
}