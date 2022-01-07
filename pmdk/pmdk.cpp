#include "pmdk.h"

PMEMobjpool *pool_;

void pobj_before_tm_start() {
  TOID(struct pool_root) Root;

  FILE *r = fopen(POOL_PATH, "r");
  if (r == NULL) {
    pool_ = pmemobj_create(POOL_PATH, LAYOUT_NAME, POOL_SIZE, 0666);
    TX_BEGIN(pool_) {
      Root = POBJ_ROOT(pool_, struct pool_root);
    }TX_END
  }
  // fclose(r);
}

void pobj_before_store(void *addr, size_t len) {
  pmemobj_tx_add_range_direct(addr, len);
}

void pobj_after_tm_end(){
  pmemobj_close(pool_);
}

void *pobj_malloc(size_t len) {
  PMEMoid oid;
  pmemobj_zalloc(pool_,&oid,len,1);
  return pmemobj_direct(oid);
}

void pobj_free(void *addr) {
  PMEMoid oid = pmemobj_oid(addr);
  pmemobj_free(&oid);
}

void *pobj_tx_malloc(size_t len) {
  return pmemobj_direct(pmemobj_tx_alloc(len,1));
}

void pobj_tx_free(void *addr) {
  pmemobj_tx_free(pmemobj_oid(addr));
}