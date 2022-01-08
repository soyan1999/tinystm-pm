#ifndef _PAGE_H_
#define _PAGE_H_

#include "pmem.h"
#include "global.h"
#include "vlog.h"

#define VADDR_TO_PADDR(vaddr)     ((void*)((uint64_t)vaddr-(uint64_t)pstm_dram_ptr+(uint64_t)pstm_nvram_ptr))
#define PADDR_TO_VADDR(paddr)     ((void*)((uint64_t)paddr+(uint64_t)pstm_dram_ptr-(uint64_t)pstm_nvram_ptr))

#ifdef __cplusplus
extern "C" {
#endif

extern void *pstm_dram_ptr;

void pstm_dram_create();
void pstm_dram_close();

#ifdef __cplusplus
}
#endif

#endif