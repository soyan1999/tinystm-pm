#ifndef _PAGE_H_
#define _PAGE_H_

#include "pmem.h"
#include "global.h"
#include "vlog.h"

extern void *pstm_dram_ptr;

#define VADDR_TO_PADDR(vaddr)     ((void*)((uint64_t)vaddr-(uint64_t)pstm_dram_ptr+(uint64_t)pstm_nvram_ptr))
#define PADDR_TO_VADDR(paddr)     ((void*)((uint64_t)paddr+(uint64_t)pstm_dram_ptr-(uint64_t)pstm_nvram_ptr))
#define IS_PMEM(addr)             ((uint64_t)addr>=(uint64_t)pstm_dram_ptr&&(uint64_t)addr<(uint64_t)pstm_dram_ptr+PSTM_SHARE_HEAP_SIZE+PSTM_HEAP_SIZE_PER_THREAD*thread_count)

// #ifdef __cplusplus
// extern "C" {
// #endif



void pstm_dram_create();
void pstm_dram_close();

// #ifdef __cplusplus
// }
// #endif

#endif