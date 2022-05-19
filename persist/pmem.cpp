#include "pmem.h"
#include "vlog.h"

void *pstm_nvram_ptr = NULL;
void *pstm_nvram_heap_ptr = NULL;
void *pstm_nvram_logs_ptr = NULL;
void *pstm_nvram_logs_root_ptr;
void *pstm_nvram_priv_heap_ptr = NULL;

__thread void *pstm_nvram_pri_ptr = NULL;


static void *alocateInNVRAM(const char *memRegion, const char *file, size_t bytes, long mapFlag, void *addr)
{
  char fileNameBuffer[1024];
  void *res = NULL;
  int fd;
  
  sprintf(fileNameBuffer, "%s%s", memRegion, file);
  fd = open(fileNameBuffer, O_CREAT | O_TRUNC | O_RDWR, 0666);
  close(fd); // writes the permissions
  fd = open(fileNameBuffer, O_CREAT | O_RDWR, 0666);

  if (fd == -1) {
    fprintf(stderr, "Error open file %s: %s\n", fileNameBuffer, strerror(errno));
  }

  // TODO: ftruncate needed after munmap...
  if (ftruncate(fd, bytes)) { // if address != NULL there was a ftruncate before
    fprintf(stderr, "Error ftruncate file %s: %s\n", fileNameBuffer, strerror(errno));
  }

  if (addr != NULL) {
    res = mmap(addr, bytes, PROT_READ | PROT_WRITE, mapFlag | MAP_FIXED, fd, 0);
    if (res != addr) {
      fprintf(stderr, "Error getting the requested address %p (got %p): %s\n", addr, res, strerror(errno));
    }
  } else {
    res = mmap(NULL, bytes, PROT_READ | PROT_WRITE, mapFlag, fd, 0);
  }
  if (res == (void*)-1 || res == NULL) {
    fprintf(stderr, "Error mmapping file %s: %s\n", fileNameBuffer, strerror(errno));
  }
  return res;
}

void pstm_nvm_create(int numThread) {
  if (FLUSHER_TYPE == 0) flusher_count = numThread;
  else if (FLUSHER_TYPE == 1) flusher_count = 1;
  else flusher_count = flusher_count;
  
  pstm_nvram_ptr = alocateInNVRAM("/mnt/pmem0/ysha/tinystm-pm/", "nvmalloc_file_shar_heap",
    PSTM_SHARE_HEAP_SIZE +
    PSTM_HEAP_SIZE_PER_THREAD * numThread,
    // PSTM_LOG_SIZE_per_thread * sizeof(w_entry_t) * 64,
  /*MAP_SHARED_VALIDATE|MAP_SYNC*/MAP_SHARED, NULL);
  pstm_nvram_heap_ptr = pstm_nvram_ptr;
  pstm_nvram_priv_heap_ptr = (void*)(((uintptr_t)pstm_nvram_ptr) + PSTM_SHARE_HEAP_SIZE);
  #ifndef LOG_USE_DRAM
  pstm_nvram_logs_root_ptr = alocateInNVRAM("/mnt/pmem0/ysha/tinystm-pm/", "nvmalloc_file_shar_log",
    sizeof(log_root)*flusher_count + PSTM_LOG_SIZE,
  /*MAP_SHARED_VALIDATE|MAP_SYNC*/MAP_SHARED, NULL);
  #else
  pstm_nvram_logs_root_ptr = mmap(NULL , sizeof(log_root)*flusher_count + PSTM_LOG_SIZE, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  #endif
  pstm_nvram_logs_ptr = (void*)(((uintptr_t)pstm_nvram_logs_root_ptr) + sizeof(log_root)*flusher_count);
}

void pstm_nvm_close() {
  int ret = munmap(pstm_nvram_ptr, PSTM_SHARE_HEAP_SIZE + PSTM_HEAP_SIZE_PER_THREAD * thread_count);
  assert(ret == 0);
  ret = munmap(pstm_nvram_logs_root_ptr, sizeof(log_root)*flusher_count + PSTM_LOG_SIZE);
  assert(ret == 0);
}

void *pstm_nvmalloc(long size)
{
  // void **nvram_mem_pos = &pstm_nvram_heap_ptr;
  // uint64_t* nvram_ptr = *((uint64_t**)nvram_mem_pos);
  if (size & 0x7) { // min 8B granularity
    size += 8;
    size &= ~0x7L;
  }
  void *ptr = (void*)__sync_fetch_and_add(&pstm_nvram_heap_ptr, size);
  if ((uintptr_t)ptr + size > (uintptr_t)pstm_nvram_ptr + PSTM_SHARE_HEAP_SIZE) {
    printf("[nvmalloc]: out of space\n");
    return (void*)-1;
  }
  #ifdef PMEM_CHECK
  memset(ptr,0,size);
  memset(PADDR_TO_VADDR(ptr),0,size);
  #endif
  return ptr;
}

void *pstm_local_nvmalloc(int threadId, long size)
{
  uintptr_t base = (uintptr_t)pstm_nvram_priv_heap_ptr + PSTM_HEAP_SIZE_PER_THREAD * threadId;

  if (pstm_nvram_pri_ptr == NULL) {
    pstm_nvram_pri_ptr = (void*)((uintptr_t)pstm_nvram_priv_heap_ptr + PSTM_HEAP_SIZE_PER_THREAD * threadId);
  }

  // void **nvram_mem_pos = &pstm_nvram_pri_ptr;
  // uint64_t* nvram_ptr = *((uint64_t**)nvram_mem_pos);
  if (size & 0x7L) { // min 8B granularity
    size += 8L;
    size &= ~0x7L;
  }
  void *ptr = (void*)__sync_fetch_and_add(&pstm_nvram_pri_ptr, size);
  if ((uintptr_t)ptr + size > base + PSTM_HEAP_SIZE_PER_THREAD) {
    printf("[local_nvmalloc %i]: out of space\n", threadId);
    return (void*)-1;
  }
  #ifdef PMEM_CHECK
  memset(ptr,0,size);
  memset(PADDR_TO_VADDR(ptr),0,size);
  #endif
  return ptr;
}

void pstm_nvm_check() {
  #ifdef PMEM_CHECK
  for (uint64_t i = 0; i < PSTM_SHARE_HEAP_SIZE + PSTM_HEAP_SIZE_PER_THREAD * thread_count; i += PAGE_SIZE>>6) {
    assert(memcmp((void *)((uint64_t)pstm_nvram_ptr+i),PADDR_TO_VADDR((void *)((uint64_t)pstm_nvram_ptr+i)),PAGE_SIZE>>6) == 0);
  }
  #endif
}