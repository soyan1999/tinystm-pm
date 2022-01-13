#include "plog.h"
#include "vlog.h"
#include "persist.h"
#include "rdtsc.h"
#include <unordered_map>

class CombineTable {
 public:
  const uint64_t min_flush_tx_count = UINT64_MAX;
  const uint64_t min_flush_w_tx_count = 20;
  const uint64_t min_flush_log_count = UINT64_MAX;
  const uint64_t min_flush_time_count = UINT64_MAX;

  uint64_t ts_start;
  uint64_t ts_end;
  uint64_t tx_count;
  uint64_t time_start;
  std::unordered_map<uint64_t,uint64_t> cb_table;

  CombineTable() {
    clear();
  }

  void clear() {
    ts_start = 0;
    ts_end = 0;
    tx_count = 0;
    time_start = 0;
    cb_table.clear();
  }

  void insert(int thread_id) {
    if (ts_start == 0 && ts_end == 0) {
      ts_start = pstm_vlogs[thread_id].ts;
      ts_end = ts_start;
      // TODO: acquire time start
    }
    if(ts_start == 0) return;
    
    if (pstm_vlogs[thread_id].log_count != 0) {
      for (uint64_t i = 0; i < pstm_vlogs[thread_id].log_count; i ++) {
        uint64_t addr = pstm_vlogs[thread_id].buffer[i*2];
        uint64_t value = pstm_vlogs[thread_id].buffer[i*2+1];
        cb_table.insert(std::unordered_map<uint64_t,uint64_t>::value_type(addr,value));
      }
      tx_count ++;
    }

    ts_end ++;

    if (ts_end - ts_start >= min_flush_tx_count || \
        tx_count >= min_flush_w_tx_count || \
        cb_table.size() >= min_flush_log_count)
    {
      //TODO: mutithread
      pstm_size_flush += cb_table.size();
      ts2 = rdtscp();
      flush_log();
      ts3 = rdtscp();
      apply_log();
      clear();
    }


  }

  void flush_log() {
    log_root_t *log_root = (log_root_t *)pstm_nvram_logs_root_ptr;
    uint64_t log_ptr_start = log_root->log_end_off;
    uint64_t log_ptr = log_ptr_start;

    ((uint64_t*)pstm_nvram_logs_ptr)[log_ptr] = BEGING_FLAG(ts_start);
    INC_LOG_PTR(log_ptr);
    for (auto i = cb_table.begin(); i != cb_table.end(); ++i) {
      ((uint64_t*)pstm_nvram_logs_ptr)[log_ptr] = i->first;
      INC_LOG_PTR(log_ptr);
      ((uint64_t*)pstm_nvram_logs_ptr)[log_ptr] = i->second;
      INC_LOG_PTR(log_ptr);
    }
    ((uint64_t*)pstm_nvram_logs_ptr)[log_ptr] = END_FLAG(cb_table.size());
    INC_LOG_PTR(log_ptr);

    FLUSH_RANGE((uint64_t *)pstm_nvram_logs_ptr+log_ptr_start, (uint64_t *)pstm_nvram_logs_ptr+log_ptr, (uint64_t *)pstm_nvram_logs_ptr, (uint64_t *)pstm_nvram_logs_ptr+(PSTM_LOG_SIZE>>3))
    FENCE_PREV_FLUSHES();

    log_root->log_end_off = log_ptr;
    FLUSH_CL(&log_root->log_end_off);
    FENCE_PREV_FLUSHES();
  }

  void apply_log() {
    log_root_t *log_root = (log_root_t *)pstm_nvram_logs_root_ptr;
    uint64_t log_ptr_start = log_root->log_start_off;
    uint64_t log_ptr_end = log_root->log_end_off;
    uint64_t log_ptr = log_ptr_start;

    if(log_ptr_end != log_ptr_start) {
      uint64_t ts = ((uint64_t*)pstm_nvram_logs_ptr)[log_ptr];
      INC_LOG_PTR(log_ptr);

      uint64_t flag = ((uint64_t*)pstm_nvram_logs_ptr)[log_ptr];
      while(!IS_END_FLAG(flag)) {
        uint64_t addr = ((uint64_t*)pstm_nvram_logs_ptr)[log_ptr];
        INC_LOG_PTR(log_ptr);
        uint64_t value = ((uint64_t*)pstm_nvram_logs_ptr)[log_ptr];
        INC_LOG_PTR(log_ptr);

        ((uint64_t*)pstm_nvram_ptr)[addr>>3] = value;
        FLUSH_CL((uint64_t*)pstm_nvram_ptr+(addr>>3));
        flag = ((uint64_t*)pstm_nvram_logs_ptr)[log_ptr];
      }
      FENCE_PREV_FLUSHES();

      INC_LOG_PTR(log_ptr);
      log_root->log_start_off = log_ptr;
      FLUSH_CL(&log_root->log_start_off);
      FENCE_PREV_FLUSHES();
    }

  }

};

static CombineTable *combine_table = NULL;

void pstm_plog_init() {
  log_root_t *log_root = (log_root_t *)pstm_nvram_logs_root_ptr;
  // if (log_root->crash) {
  //   // TODO
  // }
  // else {
    log_root->log_end_off = 0;
    log_root->log_start_off = 0;
    FLUSH_CL(&log_root->log_start_off);
    FLUSH_CL(&log_root->log_end_off);
    FENCE_PREV_FLUSHES();
    log_root->crash = 1;
    FLUSH_CL(&log_root->crash);
    FENCE_PREV_FLUSHES();
  // }
}

void pstm_plog_collect() {
  if (combine_table == NULL) combine_table = new CombineTable();
  combine_table->insert(thread_id);
}

void pstm_plog_end() {
  log_root_t *log_root = (log_root_t *)pstm_nvram_logs_root_ptr;
  
  log_root->crash = 0;
  FLUSH_CL(&log_root->crash);
  FENCE_PREV_FLUSHES();
}