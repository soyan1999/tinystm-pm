#include "plog.h"
#include "vlog.h"
#include "persist.h"
#include "rdtsc.h"
#include <bits/stdc++.h>
#include <folly/SpinLock.h>


__thread uint64_t last_persist_ts = 0;

// TODO: use std::atomic?
// sync tx thread and log thread
volatile bool pstm_stop_signal = false;



class LogFlusher {
 public:
  static const uint64_t min_flush_log_count = 31;
  static const uint64_t max_flush_log_count = 127;
  static const uint64_t min_flush_tx_count = 5;
  static const uint64_t max_flush_tx_count = 100;
  static const uint64_t min_flush_duration = 10000;
  static const uint64_t max_flush_duration = 100000;

  static const int total_flusher_num = 1;
  static const uint64_t log_area_size = PSTM_LOG_SIZE / total_flusher_num;
  static const uint64_t log_page_num = log_area_size / PAGE_SIZE;

  static const uint64_t wait_vlog_duration = 1000000;

  int flusher_id;
  log_root_t *log_root_ptr;// TODO: update log_root after flush
  void *log_start_ptr;

  ReadyVlogCollecter *ready_vlog_collecter;
  FreeVlogCollecter *free_vlog_collecter;

  // flush condition
  std::chrono::steady_clock::time_point tx_oldest_time;

  uint64_t tx_count;
  uint64_t log_count;
  // write and persist relate
  uint64_t last_persist_offset;
  uint64_t flush_offset;
  uint64_t last_collect_ts;
  std::atomic_uint64_t oldest_no_persist_ts; //TODO:update after collect and flush

  // monotonic read
  std::atomic_bool monotonic_signal = false;
  volatile uint64_t monotonic_read_ts;
  volatile uint64_t last_persist_ts;
  folly::SpinLock monotonic_signal_lock;

  // sync log thread and replay thread
  std::atomic_bool log_end_signal = false; // TODO: update after log end

  LogFlusher(int flusher_id):flusher_id(flusher_id) {
    log_start_ptr = (void *)((uint64_t)pstm_nvram_logs_ptr + flusher_id * PAGE_SIZE);
    tx_count = 0;
    log_count = 0;
    last_persist_offset = 0;
    flush_offset = 0;
  }

  inline void* gen_plog_ptr(uint64_t offset) {
    // return (void *)((offset / PAGE_SIZE) * total_flusher_num * PAGE_SIZE + offset % PAGE_SIZE + (uint64_t)log_start_ptr);
    if (total_flusher_num == 1) return ptr_add(log_start_ptr, offset);
    return (void *)((((offset>>12)&(log_page_num-1)) * total_flusher_num << 12) + (offset&(PAGE_SIZE-1)) + (uint64_t)log_start_ptr);
  }

  inline void* ptr_add(void* ptr, uint64_t offset) {
    return (void *)((uint64_t)ptr + offset);
  }

  inline uint64_t get_replay_offset() {
    return log_root_ptr->log_end_off;
  }

  void write_entry(void *entry, uint64_t size) {
    if (size != 0) {
      uint64_t step_size = PAGE_SIZE - flush_offset % PAGE_SIZE;

      while (size >= step_size) {
        memcpy(gen_plog_ptr(flush_offset), entry, step_size);
        entry = ptr_add(entry, step_size);
        flush_offset += step_size;
        step_size = PAGE_SIZE;
        size -= step_size;
      }
      if (size > 0) {
        memcpy(gen_plog_ptr(flush_offset), entry, size);
        flush_offset += size;
      }
    }
  }

  void flush_entry() {
    uint64_t flush_left = flush_offset - last_persist_offset;
    uint64_t flush_off = last_persist_offset;
    if (flush_left != 0) {
      uint64_t step_size = PAGE_SIZE - last_persist_offset;

      while (flush_left >= step_size) {
        void *flush_ptr = gen_plog_ptr(flush_off);
        FLUSH_BLOCK(flush_ptr, ptr_add(flush_ptr, step_size));
        flush_off += step_size;
        step_size = PAGE_SIZE;
        flush_left -= step_size;
      }
      if (flush_left > 0) {
        void *flush_ptr = gen_plog_ptr(flush_off);
        FLUSH_BLOCK(flush_ptr, ptr_add(flush_ptr, flush_left));
      }
    }
  }

  inline bool need_flush() {
    int ret = 0;
    auto log_delay = std::chrono::steady_clock::now() -  tx_oldest_time;
    ret -= log_delay.count()<min_flush_duration?8:0;
    ret += log_delay.count()>=max_flush_duration?8:0;
    ret -= log_count<min_flush_log_count?4:0;
    ret += log_count>=max_flush_log_count?4:0;
    ret -= tx_count<min_flush_tx_count?2:0;
    ret += tx_count>=max_flush_tx_count?2:0;

    return (ret >0);
  }

  void block_until_persist(uint64_t ts) {
    while (last_persist_ts < ts) {
      bool lk = monotonic_signal_lock.try_lock();
      if (lk) {
        monotonic_signal = true;
        monotonic_read_ts = ts;
        while (last_persist_ts < ts);
        monotonic_signal = false;
        monotonic_signal_lock.unlock();
        break;
      }
    }
  }

  void do_flush_vlog(pstm_vlog_t *vlog) {
    write_entry(&(vlog->ts), 2*sizeof(uint64_t));
    write_entry(vlog->buffer,vlog->log_count*2*sizeof(uint64_t));
    flush_entry();
  }

  virtual void do_flush_thread();

};

class CombinedLogFlusher:LogFlusher {
 public:
  std::unordered_map<uint64_t,uint64_t> cb_table;
  

  CombinedLogFlusher(int flusher_id, ReadyVlogCollecter *q1, FreeVlogCollecter *q2):LogFlusher(flusher_id) {
    ready_vlog_collecter = q1;
    free_vlog_collecter = q2;
  }

  void do_flush_cb_table() {
    uint64_t log_head[2] = {last_collect_ts, cb_table.size()};
    write_entry(log_head, 2*sizeof(uint64_t));
    for (auto it = cb_table.begin(); it != cb_table.end(); ++it) {
      write_entry(&(*it), 2*sizeof(uint64_t));
    }
    flush_entry();

    cb_table.clear();
    tx_count = 0;
    last_persist_ts = last_collect_ts;
  }

  void collect_vlog(pstm_vlog_t *vlog) {
    if (tx_count == 0) tx_oldest_time = std::chrono::steady_clock::now();
    uint64_t *log_ptr = vlog->buffer;
    for (int i = 0; i < vlog->log_count; i ++) {
      cb_table[*log_ptr] = *(log_ptr + 1);
      log_ptr += 2;
    }
    last_collect_ts = vlog->ts;
    tx_count ++;
  }

  virtual void do_flush_thread() { // TODO : update log_root
    while (!pstm_stop_signal && ready_vlog_collecter->empty()) {
      // read ts in cb_table
      if (monotonic_signal.load() && last_collect_ts >= monotonic_read_ts && tx_count != 0) {
        do_flush_cb_table();
      }
      pstm_vlog_t *vlog = ready_vlog_collecter->get(wait_vlog_duration);
      if (vlog != nullptr) {
        collect_vlog(vlog);
        free_vlog_collecter->put(vlog);
        if (need_flush()) do_flush_cb_table();
      }
    }
  }
};

class LogReplayer {
 public:
  std::vector<uint64_t> last_replay_ts;//TODO:usage?
  std::vector<LogFlusher*> log_flushers;
  std::list<int> stall_flusher;

  bool recover_flag = false;



  // ts and log_flusher id(<<2) pair, the lower one bit indicate if the ts log no persist 
  std::priority_queue<std::pair<uint64_t,int>,std::vector<std::pair<uint64_t,int>>,std::greater<std::pair<uint64_t,int>>> next_queue;

  LogReplayer():last_replay_ts(LogFlusher::total_flusher_num),log_flushers(LogFlusher::total_flusher_num, nullptr) {

  }

  inline void* gen_log_ptr(int flusher_id, uint64_t offset) {
    return log_flushers[flusher_id]->gen_plog_ptr(offset);
  }
  
  inline uint64_t get_log_off(int flusher_id) {
    return ((log_root_t *)log_flushers[flusher_id]->log_root_ptr)->log_start_off;
  }

  inline uint64_t get_log_data(int flusher_id, uint64_t offset) {
    return *((uint64_t *)gen_log_ptr(flusher_id, offset));
  }

  inline uint64_t get_log_ts(int flusher_id) {
    uint64_t offset = ((log_root_t *)log_flushers[flusher_id]->log_root_ptr)->log_start_off;
    return get_log_data(flusher_id, offset);
  }

  void do_replay(int flusher_id) {
    uint64_t replay_offset = ((log_root_t *)log_flushers[flusher_id]->log_root_ptr)->log_start_off;
    uint64_t ts = get_log_data(flusher_id, replay_offset);
    replay_offset += 8;
    uint64_t log_size = get_log_data(flusher_id, replay_offset);
    replay_offset += 8;

    for (uint64_t i = 0; i < log_size; i++) {
      uint64_t nvm_addr = get_log_data(flusher_id, replay_offset);
      replay_offset += 8;
      uint64_t value = get_log_data(flusher_id, replay_offset);
      replay_offset += 8;
      ((uint64_t*)pstm_nvram_heap_ptr)[nvm_addr>>3] = value;

      FLUSH_CL(pstm_nvram_heap_ptr + (nvm_addr>>3));
    }

    FENCE_PREV_FLUSHES();
    ((log_root_t *)log_flushers[flusher_id]->log_root_ptr)->log_start_off = replay_offset;
    FLUSH_CL(&(((log_root_t *)log_flushers[flusher_id]->log_root_ptr)->log_start_off));
    FENCE_PREV_FLUSHES();

    last_replay_ts[flusher_id] = ts;
  }

  int get_next_flusher_id() {
    if (LogFlusher::total_flusher_num == 1) {
      if (last_replay_ts[0] >= log_flushers[0]->last_persist_ts && log_flushers[0]->log_end_signal.load()) return -1;
      else return 0;
    }
    else {
      // ASSERT(next_queue.size() == LogFlusher::total_flusher_num);
      // check stall flusher , if no stall add to heap
      for (auto it = stall_flusher.begin(); it != stall_flusher.end(); ++it) {
        if (!log_flushers[*it]->ready_vlog_collecter->empty() || log_flushers[*it]->oldest_no_persist_ts.load() > last_replay_ts[*it]) {
          while(true) {
            uint64_t next_ts = get_log_ts(*it);
            if (next_ts >= last_replay_ts[*it]) {
              next_queue.push({next_ts, (*it)<<1});
              stall_flusher.erase(it);
              break;
            }
          }
          // TODO:if oldest_no_persist_ts changed
          // else {
          //   while ((next_ts = log_flushers[*it]->oldest_no_persist_ts.load()) <= last_replay_ts[*it]);
          //   next_queue.push({next_ts, ((*it)<<1)|1});
          // }
        }
      }
      auto rt = next_queue.top();
      if (rt.first == UINT64_MAX) {
        // log end
        return -1;
      }
      else {
        uint64_t ts = rt.first, next_ts;
        int flusher_id = rt.second >> 1; 
        int not_persist = rt.second & 1;
        // wait until persist
        while(not_persist) {
          if (log_flushers[flusher_id]->last_persist_ts >= ts) not_persist = false;
        }
        // if next ts ready
        if (log_flushers[flusher_id]->last_persist_ts > ts) {
          next_ts = get_log_ts(flusher_id);
          next_queue.push({next_ts, flusher_id<<1});
        }
        // not ready but in group
        // TODO: need check again?
        else if ((next_ts = log_flushers[flusher_id]->oldest_no_persist_ts.load()) > ts) {
          next_queue.push({next_ts, (flusher_id<<1)|1});
        }
        // not ready not in group no end
        else if (!log_flushers[flusher_id]->log_end_signal.load()) {
          stall_flusher.push_back(flusher_id);
        }
        // stoped
        else {
          next_queue.push({UINT64_MAX,flusher_id<<1});
        }
      }
    }
  }

  virtual void do_replay_thread() {
    while (true) {
      int flusher_id = get_next_flusher_id();
      if (flusher_id == -1) break;
      else {
        do_replay(flusher_id);
      }
    }
  }


};
/*
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
      // ts_end = ts_start;
      // TODO: acquire time start
    }
    if(ts_start == 0) return;
    
    ts_end = pstm_vlogs[thread_id].ts;
    if (pstm_vlogs[thread_id].log_count != 0) {
      for (uint64_t i = 0; i < pstm_vlogs[thread_id].log_count; i ++) {
        uint64_t addr = pstm_vlogs[thread_id].buffer[i*2];
        uint64_t value = pstm_vlogs[thread_id].buffer[i*2+1];
        cb_table.insert_or_assign(addr,value);
      }
      tx_count ++;
    }

    // ts_end ++;

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
      // ts4 = rdtscp();
      last_persist_ts = ts_end;
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

static CombineTable *combine_table = new CombineTable();

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
  // if (combine_table == NULL) combine_table = (CombineTable *)malloc(sizeof(CombineTable));
  combine_table->insert(thread_id);
}

void pstm_plog_end() {
  log_root_t *log_root = (log_root_t *)pstm_nvram_logs_root_ptr;
  
  log_root->crash = 0;
  FLUSH_CL(&log_root->crash);
  FENCE_PREV_FLUSHES();
}
*/