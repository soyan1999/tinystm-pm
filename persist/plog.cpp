#include "plog.h"
#include "vlog.h"
#include "persist.h"
#include "rdtsc.h"
#include <bits/stdc++.h>
// #include <folly/SpinLock.h>
#include <mutex>
#include <emmintrin.h>


// __thread uint64_t last_persist_ts = 0;

// LogFlusher **log_flushers;
// LogReplayer *log_replayer;

std::vector<std::thread> log_thread_pool;

// TODO: use std::atomic?
// sync tx thread and log thread
std::atomic_bool pstm_stop_signal = false;

int flusher_count;



class LogFlusher {
 public:
  static const uint64_t min_flush_log_count = 31;
  static const uint64_t max_flush_log_count = 127;
  static const uint64_t min_flush_tx_count = 5;
  static const uint64_t max_flush_tx_count = READY_VLOG_PER_FLUSHER;
  static const int64_t min_flush_duration = 10000;
  static const int64_t max_flush_duration = 100000;

  // static int flusher_count;
  static uint64_t log_area_size;
  static uint64_t log_page_num; //should be 2^n
  static LogFlusher **flushers;

  static const uint64_t wait_vlog_duration = 1000000;

  int flusher_id;
  log_root_t *log_root_ptr;// TODO: update log_root after flush
  void *log_start_ptr;

  ReadyVlogCollecter *ready_vlog_collecter;
  // std::vector<FreeVlogCollecter*> free_vlog_collecters;

  // flush condition
  std::chrono::steady_clock::time_point tx_oldest_time;

  uint64_t tx_count;
  uint64_t log_count;
  // write and persist relate
  uint64_t last_persist_offset;
  uint64_t flush_offset;
  uint64_t last_collect_ts;
  // std::atomic_uint64_t oldest_no_persist_ts; //TODO:update after collect and flush
  std::atomic_uint64_t replay_offset;
  // pstm_vlog_t *oldest_no_persist_vlog;

  // monotonic read
  std::atomic_bool monotonic_signal = false;
  volatile uint64_t monotonic_read_ts;
  std::atomic_uint64_t last_persist_ts;
  std::mutex monotonic_signal_lock;

  // sync log thread and replay thread
  std::atomic_bool log_end_signal = false; // TODO: update after log end

  std::list<uint64_t> persist_ts;

  LogFlusher(int flusher_id):flusher_id(flusher_id) {
    log_start_ptr = (void *)((uint64_t)pstm_nvram_logs_ptr + flusher_id * log_area_size);
    tx_count = 0;
    log_count = 0;
    last_collect_ts = 0;
    last_persist_offset = 0;
    flush_offset = 0;
    // oldest_no_persist_ts = 0;
    last_persist_ts = 0;
    replay_offset = 0;

    log_root_ptr = (log_root_t*)ptr_add(pstm_nvram_logs_root_ptr, flusher_id*sizeof(log_root_t)); 
    ready_vlog_collecter = ready_vlog_collecters[flusher_id];
  }

  inline void* gen_plog_ptr(uint64_t offset) {
    // return (void *)((offset / PAGE_SIZE) * flusher_count * PAGE_SIZE + offset % PAGE_SIZE + (uint64_t)log_start_ptr);
    // if (flusher_count == 1) return ptr_add(log_start_ptr, offset);
    // return (void *)((((offset>>12)%log_page_num) * (flusher_count << 12)) + (offset&(PAGE_SIZE-1)) + (uint64_t)log_start_ptr);
    return ptr_add(log_start_ptr, offset % log_area_size);
  }

  inline void* ptr_add(void* ptr, uint64_t offset) {
    return (void *)((uint64_t)ptr + offset);
  }

  inline bool wait_vlog_committed(pstm_vlog_t *vlog) {
    uint64_t state;
    while ((state = vlog->state.load()) == VLOG_PRE_COMMIT);
    if (state == VLOG_COMMITTED) return true;
    else return false;
  }

  uint64_t get_log_offset(uint64_t ts) {
    uint64_t start_off = last_persist_offset;
    uint64_t start_ts, log_count, dep_count;
    if ((start_ts = last_persist_ts) >= ts) return UINT64_MAX;
    while (start_ts < ts) {
      log_count = *((uint64_t *)(gen_plog_ptr(start_off)) + 1);
      start_off += ((log_count&UINT32_MAX)+(log_count>>32)) * 16 + 16;
      start_ts = *((uint64_t *)(gen_plog_ptr(start_off)));
    }
    if (start_ts == ts) return start_off;
    else return UINT64_MAX;
  }

  void write_entry(void *entry, uint64_t size) {
    // block until log_space avaliable
    while (flush_offset - replay_offset.load() > log_area_size - size);

    #ifndef USE_NTSTORE
    if (size != 0) {
      // uint64_t step_size = PAGE_SIZE - (flush_offset & (PAGE_SIZE - 1));

      // while (size >= step_size) {
      //   memcpy(gen_plog_ptr(flush_offset), entry, step_size);
      //   entry = ptr_add(entry, step_size);
      //   flush_offset += step_size;
        
      //   size -= step_size;
      //   step_size = PAGE_SIZE;
      // }
      // if (size > 0) {
      //   memcpy(gen_plog_ptr(flush_offset), entry, size);
      //   flush_offset += size;
      // }
      assert(size <= log_area_size);
      if (flush_offset % log_area_size + size <= log_area_size) {
        memcpy(gen_plog_ptr(flush_offset), entry, size);
      }
      else {
        memcpy(gen_plog_ptr(flush_offset), entry, log_area_size - flush_offset % log_area_size);
        size -= log_area_size - flush_offset % log_area_size;
        entry = ptr_add(entry, log_area_size - flush_offset % log_area_size);
        memcpy(gen_plog_ptr(flush_offset), entry, size);
      }
      flush_offset += size;
    }
    #else
    if (size != 0) {
      uint64_t step_size = PAGE_SIZE - (flush_offset & (PAGE_SIZE - 1));

      while (size >= step_size) {
        __m128i *dst = (__m128i *)gen_plog_ptr(flush_offset), *src = (__m128i *)entry;
        for (int i = 0; i < step_size; i += 128) {
          _mm_stream_si128(dst, *src);
          dst++;
          src++;
        }
        entry = ptr_add(entry, step_size);
        flush_offset += step_size;
        
        size -= step_size;
        step_size = PAGE_SIZE;
      }
      if (size > 0) {
        __m128i *dst = (__m128i *)gen_plog_ptr(flush_offset), *src = (__m128i *)entry;
        for (int i = 0; i < size; i += 128) {
          _mm_stream_si128(dst, *src);
          dst++;
          src++;
        }
        flush_offset += size;
      }
    }
    #endif
  }

  uint64_t flush_tx(uint64_t offset) {
    uint64_t log_count = *((uint64_t*)gen_plog_ptr(offset) + 1), dep_count;
    dep_count = log_count >> 32;
    log_count = log_count&UINT32_MAX;
    for (size_t i = log_count; i < log_count+dep_count; i ++) {
      uint64_t lock_val = *((uint64_t*)gen_plog_ptr(offset) + 2*(i+1));
      // dep chain
      if (lock_val & 0x1) {
        uint64_t dep_ts = lock_val >> 9;
        int dep_thread_id = (lock_val >> 4) & ((1 << 5) - 1);
        if (dep_thread_id != flusher_id && flushers[flusher_id]->last_persist_ts < dep_ts) {
          uint64_t dep_offset = flushers[flusher_id]->get_log_offset(dep_ts);
          if (dep_offset != UINT64_MAX) flushers[flusher_id]->flush_tx(dep_offset);
        }
      }
    }
    uint64_t flush_size = (log_count+dep_count+1)*16;
    assert(flush_size <= log_area_size);
    if (offset % log_area_size + flush_size <= log_area_size) {
      FLUSH_BLOCK(gen_plog_ptr(offset), gen_plog_ptr(offset+flush_size));
    }
    else {
      FLUSH_BLOCK(gen_plog_ptr(offset), ptr_add(log_start_ptr, log_area_size));
      flush_size -= log_area_size - offset%log_area_size;
      FLUSH_BLOCK(log_start_ptr, gen_plog_ptr(flush_size));
    }

    return offset+flush_size;
  }
  
  void flush_entry() {
    uint64_t flush_left = flush_offset - last_persist_offset;
    uint64_t flush_off = last_persist_offset;
    if (flush_left != 0) {
      #ifndef USE_NTSTORE
      #ifdef TRACE_DEP
      while (flush_off < flush_offset) {
        flush_off = flush_tx(flush_off);
      }
      #else
      uint64_t flush_size = flush_left;
      assert(flush_size <= log_area_size);
      if (flush_offset % log_area_size + flush_size <= log_area_size) {
        FLUSH_BLOCK(gen_plog_ptr(flush_offset), gen_plog_ptr(flush_offset+flush_size));
      }
      else {
        FLUSH_BLOCK(gen_plog_ptr(flush_offset), ptr_add(log_start_ptr, log_area_size));
        flush_size -= log_area_size - flush_offset%log_area_size;
        FLUSH_BLOCK(log_start_ptr, gen_plog_ptr(flush_size));
      }
      #endif
      #endif
      FENCE_PREV_FLUSHES();
      log_root_ptr->log_end_off = flush_offset;
      FLUSH_CL(&log_root_ptr->log_end_off);
      FENCE_PREV_FLUSHES();
      last_persist_offset = flush_offset;

    }
  }

  inline bool need_flush(uint64_t log_count_) {
    int ret = 0;
    auto log_delay = std::chrono::steady_clock::now() -  tx_oldest_time;
    ret -= log_delay.count()<min_flush_duration?8:0;
    ret += log_delay.count()>=max_flush_duration?8:0;
    ret -= log_count_<min_flush_log_count?4:0;
    ret += log_count_>=max_flush_log_count?4:0;
    ret -= tx_count<min_flush_tx_count?2:0;
    ret += tx_count>=max_flush_tx_count?2:0;

    return (ret >0);
  }

  void block_until_persist(uint64_t ts) {
    while (last_persist_ts < ts) {
      bool lk = monotonic_signal_lock.try_lock();
      if (lk) {
        monotonic_read_ts = ts;
        monotonic_signal = true;
        while (last_persist_ts < ts);
        monotonic_signal = false;
        monotonic_signal_lock.unlock();
        break;
      }
    }
  }

  void do_flush_vlog(pstm_vlog_t *vlog) {
    if (!wait_vlog_committed(vlog)) return;
    write_entry(&(vlog->ts), 2*sizeof(uint64_t));
    write_entry(vlog->buffer,(vlog->log_count+vlog->dep_count)*2*sizeof(uint64_t));
    tx_count ++;
    if (tx_count >= GROUP_SIZE) {
      flush_entry();
      tx_count = 0;
    }

    vlog->state.store(VLOG_PERSISTED);
    // #ifdef ORDER_COLLECT
    // ready_vlog_collecter->vlog_collect_lock.lock();
    // #endif
    last_persist_ts = vlog->ts;
    // #ifdef ORDER_COLLECT
    // ready_vlog_collecter->vlog_collect_lock.unlock();
    // #endif
  }

  // get last_ready ts
  // virtual uint64_t get_last_ready_ts() {
  //   uint64_t ts;
  //   ready_vlog_collecter->vlog_collect_lock.lock();
  //   ts = ready_vlog_collecter->last_ready_ts;
  //   ready_vlog_collecter->vlog_collect_lock.unlock();
  //   return ts;
  // }

  virtual void do_flush_thread() {}

};

class LogFlusher2: public LogFlusher {
 public:
  
};

class CombinedLogFlusher: public LogFlusher {
 public:
  std::unordered_map<uint64_t,uint64_t> cb_table;
  #ifdef PDEBUG
  uint64_t recive_count = 0;
  #endif
  CombinedLogFlusher(int flusher_id):LogFlusher(flusher_id) {
    // ready_vlog_collecter = ready_vlog_collecters[flusher_id];
    
  }

  void do_flush_cb_table(std::unordered_map<uint64_t,uint64_t> &cb_table, uint64_t last_collect_ts) {
    #ifdef ENABLE_FLUSH
    uint64_t log_head[2] = {last_collect_ts, cb_table.size()};
    write_entry(log_head, 2*sizeof(uint64_t));
    for (auto it = cb_table.begin(); it != cb_table.end(); ++it) {
      write_entry(&(*it), 2*sizeof(uint64_t));
    }
    flush_entry();
    #endif

    cb_table.clear();
    // tx_count = 0;
    last_persist_ts = last_collect_ts;
  }

  bool collect_vlog(pstm_vlog_t *vlog, std::unordered_map<uint64_t,uint64_t> &cb_table) {
    if (tx_count == 0) {
      tx_oldest_time = std::chrono::steady_clock::now();
      // oldest_no_persist_ts.store(vlog->ts);
    }

    // if tx abort just skip
    if (!wait_vlog_committed(vlog)) return false;

    uint64_t *log_ptr = vlog->buffer;
    #ifdef ENABLE_COMBINE
    for (uint64_t i = 0; i < vlog->log_count; i ++) {
      cb_table[*log_ptr] = *(log_ptr + 1);
      log_ptr += 2;
    }
    #endif
    #ifdef ORDER_COLLECT
    ASSERT(last_collect_ts < vlog->ts);
    #endif
    last_collect_ts = vlog->ts;
    tx_count ++;

    #ifdef PDEBUG
    recive_count += vlog->log_count;
    #endif

    return true;
  }

  virtual void do_flush_thread() { // TODO : update log_root
    while (true) {
      if (pstm_stop_signal.load()) {
        if (ready_vlog_collecter->empty()) {
          break;
        }
      } 
      // read ts in cb_table
      if (monotonic_signal.load() && last_collect_ts >= monotonic_read_ts && last_persist_ts.load() < monotonic_read_ts && tx_count != 0) {
        do_flush_cb_table(cb_table, last_collect_ts);
        tx_count = 0;
      }
      pstm_vlog_t *vlog = ready_vlog_collecter->get(wait_vlog_duration);
      if (vlog != nullptr) {
        collect_vlog(vlog,cb_table);
        free_vlog_collecters[vlog->thread_id]->put(vlog);
        if (need_flush(cb_table.size())) {
          do_flush_cb_table(cb_table, last_collect_ts);
          tx_count = 0;
        }
      }
    }
    if (!cb_table.empty()) {
      do_flush_cb_table(cb_table, last_collect_ts);
      tx_count = 0;
    }
    #ifdef PDEBUG
    assert(recive_count == commit_count);
    #endif
    log_end_signal.store(true);
  }
};

class CombinedLogFlusher2:public CombinedLogFlusher {
 public:
  std::unordered_map<uint64_t,uint64_t> wrt_table;
  std::atomic_uint64_t ts_cb, ts_wrt;
  std::atomic_bool cb_stop;
  uint64_t last_collect_ts_wrt;

  CombinedLogFlusher2(int flusher_id):CombinedLogFlusher(flusher_id) {
    ts_cb = 0;
    ts_wrt = 0;
    cb_stop = false;
  }

  void do_flush_table_thread() {
    while(!cb_stop.load()) {
      while(ts_wrt.load() > ts_cb.load() - 1);
      do_flush_cb_table(wrt_table,last_collect_ts_wrt);
      ts_wrt++;
    }
    while(ts_wrt.load() < ts_cb.load()) {
      do_flush_cb_table(wrt_table,last_collect_ts_wrt);
      ts_wrt++;
    }
  }

  void end_cb_table() {
    std::swap(wrt_table,cb_table);
    last_collect_ts_wrt = last_collect_ts;
    tx_count = 0;
    ts_cb ++;
  }

  virtual void do_flush_thread() {
    std::thread flush_thd(&CombinedLogFlusher2::do_flush_table_thread, this);
    
    while (!pstm_stop_signal.load() || !ready_vlog_collecter->empty()) {
      // read ts in cb_table
      if (monotonic_signal.load() && last_collect_ts >= monotonic_read_ts && last_collect_ts_wrt < monotonic_read_ts && tx_count != 0) {
        while (ts_wrt.load() < ts_cb.load());
        end_cb_table();
      }
      pstm_vlog_t *vlog = ready_vlog_collecter->get(wait_vlog_duration);
      if (vlog != nullptr) {
        collect_vlog(vlog,cb_table);
        free_vlog_collecters[vlog->thread_id]->put(vlog);
        if (need_flush(cb_table.size())) {
          while (ts_wrt.load() < ts_cb.load());
          end_cb_table();
        }
      }
    }

    cb_stop.store(true);
    flush_thd.join();
    log_end_signal.store(true);
  }

};

class LogReplayer {
 public:
  std::vector<uint64_t> last_replay_ts;//TODO:combine update?
  uint64_t last_get_ts = 0; // used for assert
  LogFlusher** log_flushers_ptr;
  std::list<int> stall_flusher;

  bool recover_flag = false;

  std::list<uint64_t> replayed;



  // ts and log_flusher id(<<2) pair, the lower one bit indicate if the ts log no persist 
  std::priority_queue<std::pair<uint64_t,int>,std::vector<std::pair<uint64_t,int>>,std::greater<std::pair<uint64_t,int>>> next_queue;

  LogReplayer(LogFlusher** log_flushers):last_replay_ts(flusher_count),log_flushers_ptr(log_flushers) {
    for (int i = 0; i < flusher_count; i ++) {
      stall_flusher.push_back(i);
    }
  }

  inline void* gen_log_ptr(int flusher_id, uint64_t offset) {
    return log_flushers_ptr[flusher_id]->gen_plog_ptr(offset);
  }
  
  inline uint64_t get_log_off(int flusher_id) {
    return ((log_root_t *)log_flushers_ptr[flusher_id]->log_root_ptr)->log_start_off;
  }

  inline uint64_t get_log_data(int flusher_id, uint64_t offset) {
    return *((uint64_t *)gen_log_ptr(flusher_id, offset));
  }

  inline uint64_t get_log_ts(int flusher_id) {
    uint64_t offset = ((log_root_t *)log_flushers_ptr[flusher_id]->log_root_ptr)->log_start_off;
    return get_log_data(flusher_id, offset);
  }

  inline uint64_t get_next_log_ts(int flusher_id) {
    uint64_t offset = ((log_root_t *)log_flushers_ptr[flusher_id]->log_root_ptr)->log_start_off;
    uint64_t log_size = get_log_data(flusher_id, offset + sizeof(uint64_t));
    offset += 2 * sizeof(uint64_t) * (log_size + 1);
    return get_log_data(flusher_id, offset);
  }

  void do_replay(int flusher_id) {
    uint64_t replay_offset = ((log_root_t *)log_flushers_ptr[flusher_id]->log_root_ptr)->log_start_off;
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

      FLUSH_CL((uint64_t*)pstm_nvram_heap_ptr + (nvm_addr>>3));
    }

    FENCE_PREV_FLUSHES();
    ((log_root_t *)log_flushers_ptr[flusher_id]->log_root_ptr)->log_start_off = replay_offset;
    FLUSH_CL(&(((log_root_t *)log_flushers_ptr[flusher_id]->log_root_ptr)->log_start_off));
    FENCE_PREV_FLUSHES();
    log_flushers_ptr[flusher_id]->replay_offset = replay_offset;

    last_replay_ts[flusher_id] = ts;
  }

  int get_next_flusher_id() {
    // if (flusher_count == 1) {
    //   if (last_replay_ts[0] >= log_flushers_ptr[0]->last_persist_ts.load() && log_flushers_ptr[0]->log_end_signal.load()) return -1;
    //   else {
    //     while (last_replay_ts[0] >= log_flushers_ptr[0]->last_persist_ts.load()) {
    //       if (log_flushers_ptr[0]->log_end_signal.load()) return -1;
    //     }
    //     return 0;
    //   }
    // }
    // else {
      // while (true) {
      //   // ASSERT(next_queue.size() == flusher_count);
      //   // check stall flusher , if no stall add to heap
      //   for (auto it = stall_flusher.begin(); it != stall_flusher.end(); ++it) {
      //     if (FLUSHER_TYPE == 0) {
      //       if (log_flushers_ptr[*it]->last_persist_ts.load() > last_replay_ts[*it]) {
      //         uint64_t next_ts = get_log_ts(*it);
      //         if (next_ts > last_replay_ts[*it]) {
      //           next_queue.push({next_ts, (*it)<<1});
      //           stall_flusher.erase(it);
      //           it = stall_flusher.begin();
      //         }
      //       }
      //     }
      //     else if (!log_flushers_ptr[*it]->ready_vlog_collecter->empty() || log_flushers_ptr[*it]->oldest_no_persist_ts.load() > last_replay_ts[*it]) {
      //       while(true) {
      //         uint64_t next_ts = get_log_ts(*it);
      //         if (next_ts > last_replay_ts[*it]) {
      //           next_queue.push({next_ts, (*it)<<1});
      //           stall_flusher.erase(it);
      //           it = stall_flusher.begin();
      //         }
      //       }
      //       // TODO:if oldest_no_persist_ts changed
      //       // else {
      //       //   while ((next_ts = log_flushers_ptr[*it]->oldest_no_persist_ts.load()) <= last_replay_ts[*it]);
      //       //   next_queue.push({next_ts, ((*it)<<1)|1});
      //       // }
      //     }
      //     if (log_flushers_ptr[*it]->log_end_signal.load()) {
      //       stall_flusher.erase(it);
      //       it = stall_flusher.begin();
      //     }
      //   }
      //   if (next_queue.empty() && stall_flusher.empty()) {
      //     return -1;
      //   }
      //   else if(!next_queue.empty()) break;
      // }

      int flusher_id;
      uint64_t min_ts = UINT64_MAX, ts;
      // bool have_ready = false;
      // std::vector<bool> is_ready(flusher_count, false);

      // int i = 0;
      // while (i < flusher_count) {
      //   if (!is_ready[i]) {
      //     last_ready_ts = log_flushers_ptr[i]->get_last_ready_ts();
      //     if (last_ready_ts > last_replay_ts[i]) {
      //       have_ready = true;
      //       is_ready[i] = true;
      //       i = 0;
      //     }
      //     else if (i >= flusher_count - 1) {
      //       i = 0;
      //     }
      //     else {
      //       i ++;
      //     }
      //   }
      //   else i ++;
      // }

      // for (int i = 0; i < flusher_count; i ++) {
      //   if (log_flushers_ptr[i]->log_end_signal) continue;
      //   while ((last_ready_ts = log_flushers_ptr[i]->get_last_ready_ts()) < last_replay_ts[i]);
      // }
      // for (int i = 0; i < flusher_count;) {
      //   if (is_ready[i]) {
      //     i ++;
      //     continue;
      //   }
      //   last_ready_ts = log_flushers_ptr[i]->get_last_ready_ts();
      //   if (last_ready_ts > last_replay_ts[i]) {
      //     have_ready = true;
      //     is_ready[i] = true;
      //     i = 0;
      //     continue;
      //   }
      //   if (i == flusher_count - 1 && !have_ready) {
      //     i = 0;
      //     continue;
      //   }

      //   i ++;
      // }

      for (int i = 0; i < flusher_count; i ++) {
        while((ts=log_flushers_ptr[i]->last_persist_ts) <= last_replay_ts[i]) {
          if (log_flushers_ptr[i]->log_end_signal) {
            // check again
            if ((ts=log_flushers_ptr[i]->last_persist_ts) <= last_replay_ts[i]) 
              break;
          }
        }
        if (ts == last_replay_ts[i]) continue;
        else if ((ts = get_log_ts(i)) < min_ts) {
          min_ts = ts;
          flusher_id = i;
        }
      }
      if (min_ts == UINT64_MAX) return -1;
      // printf("ts:%lu\t\tflusher:%d\n", min_ts, flusher_id);

      // std::pair<uint64_t, int> rt;
      // uint64_t ts, next_ts, recent_ts;

      // if (next_queue.empty()) ts = UINT64_MAX;
      // else ts = next_queue.top().first;

      // for (auto it = stall_flusher.begin(); it != stall_flusher.end();) {
      //   recent_ts = log_flushers_ptr[*it]->get_recent_ts(last_replay_ts[*it], ts);
      //   if (recent_ts != UINT64_MAX) {
      //     next_ts = get_log_ts(*it);
      //     next_queue.push({next_ts,(*it)<<1});
      //     ts = std::min(next_ts, ts);
      //     stall_flusher.erase(it++);
      //   }
      //   // else if (recent_ts < ts && recent_ts > last_replay_ts[*it]) {
      //   //   next_ts = recent_ts;
      //   //   next_queue.push({next_ts,(*it)<<1});
      //   //   ts = next_ts;
      //   //   stall_flusher.erase(it++);
      //   // }
      //   else ++it;

      //   if (it == stall_flusher.end() && next_queue.empty()) {
      //     it = stall_flusher.begin();
      //   }
      // }

      // rt = next_queue.top();
      // next_queue.pop();
      // // if (rt.first == UINT64_MAX) {
      // //   // log end
      // //   // return -1;
      // // }
      // // else {
      //   ts = rt.first, next_ts;
      //   int flusher_id = rt.second >> 1; 
      //   int not_persist = rt.second & 1;
      //   // wait until persist
      //   while(not_persist) {
      //     if (log_flushers_ptr[flusher_id]->last_persist_ts >= ts) not_persist = false;
      //   }
      //   // if next ts ready
      //   if (log_flushers_ptr[flusher_id]->last_persist_ts > ts) {
      //     next_ts = get_next_log_ts(flusher_id);
      //     next_queue.push({next_ts, flusher_id<<1});
      //   }
      //   // not ready but in group, or on group flusher
      //   // TODO: need check again?
      //   // else if ((next_ts = log_flushers_ptr[flusher_id]->oldest_no_persist_ts.load()) > ts) {
      //   //   next_queue.push({next_ts, (flusher_id<<1)|1});
      //   // }
      //   // // not ready not in group no end
      //   // else if (!log_flushers_ptr[flusher_id]->log_end_signal.load()) {
      //   //   stall_flusher.push_back(flusher_id);
      //   // }
      //   // // stoped
      //   // else {
      //   //   next_queue.push({UINT64_MAX,flusher_id<<1});
      //   // }
      //   else {
      //     stall_flusher.push_back(flusher_id);
      //   }

        #ifdef ORDER_COLLECT
        ASSERT(min_ts > last_get_ts);
        #endif
        #ifdef PDEBUG
        replayed.push_back(min_ts);
        #endif
        last_get_ts = min_ts;
        return flusher_id;
      // }
    // }
  }

  virtual void do_replay_thread() {
    while (true) {
      int flusher_id = get_next_flusher_id();
      if (flusher_id == -1) {
        // uint64_t count = 0;
        // for (int i = 0; i < flusher_count; i ++) {
        //   count += log_flushers_ptr[i]->persist_ts.size();
        // }
        // ASSERT(count == replayed.size());
        break;
      }
      else {
        do_replay(flusher_id);
      }
    }
  }


};

class LogCombineReplayer: public LogReplayer {
 public:
  std::unordered_map<uint64_t,uint64_t> tb_combine, tb_write;
  std::vector<uint64_t> replay_off_combine, replay_off_write;
  uint64_t last_replay_ts_combine, last_replay_ts_write;
  std::atomic_uint64_t ts_cb, ts_wrt;
  std::atomic_bool replay_stop;

  LogCombineReplayer(LogFlusher** log_flushers):LogReplayer(log_flushers),replay_off_combine(flusher_count),replay_off_write(flusher_count) {
    ts_cb = 0;
    ts_wrt = 0;
    replay_stop = false;
  }
  
  void do_replay_tb() {
    for (auto it = tb_write.begin(); it != tb_write.end(); ++it) {
      ((uint64_t*)pstm_nvram_heap_ptr)[((*it).first)>>3] = (*it).second;
      FLUSH_CL((uint64_t*)pstm_nvram_heap_ptr + (((*it).first)>>3));
    }
    
    FENCE_PREV_FLUSHES();
    ((log_root_t *)log_flushers_ptr[0]->log_root_ptr)->replay_ts = last_replay_ts_write;
    FLUSH_CL(&(((log_root_t *)log_flushers_ptr[0]->log_root_ptr)->replay_ts));
    FENCE_PREV_FLUSHES();
    for (int i = 0; i < flusher_count; i ++) {
      ((log_root_t *)log_flushers_ptr[flusher_id]->log_root_ptr)->log_start_off = replay_off_write[i];
      FLUSH_CL(&(((log_root_t *)log_flushers_ptr[flusher_id]->log_root_ptr)->log_start_off));
    }
    FENCE_PREV_FLUSHES();
  }

  void do_replay_tb_thread() {
    while(!replay_stop.load()) {
      while(ts_wrt.load() > ts_cb.load() - 1);
      do_replay_tb();
      tb_write.clear();
      ts_wrt++;
    }
    while(ts_wrt.load() < ts_cb.load()) {
      do_replay_tb();
      ts_wrt++;
    }
  }

  void do_combine_log(int flusher_id) {
    uint64_t replay_offset = ((log_root_t *)log_flushers_ptr[flusher_id]->log_root_ptr)->log_start_off;
    uint64_t ts = get_log_data(flusher_id, replay_offset);
    replay_offset += 8;
    uint64_t log_size = get_log_data(flusher_id, replay_offset);
    replay_offset += 8;

    for (uint64_t i = 0; i < log_size; i++) {
      uint64_t nvm_addr = get_log_data(flusher_id, replay_offset);
      replay_offset += 8;
      uint64_t value = get_log_data(flusher_id, replay_offset);
      replay_offset += 8;
      tb_combine[nvm_addr] = value;
    }

    replay_off_combine[flusher_id] = replay_offset;
    last_replay_ts_combine = ts;

    last_replay_ts[flusher_id] = ts;
  }

  virtual void do_replay_thread() {
    std::thread th_wrt(&LogCombineReplayer::do_replay_tb_thread,this);
    while (true) {
      int flusher_id = get_next_flusher_id();
      if (flusher_id == -1) break;
      else {
        do_combine_log(flusher_id);
        if (tb_combine.size() > MAX_REPLAY_SIZE) {
          while (ts_wrt.load() < ts_cb.load());
          std::swap(tb_combine, tb_write);
          replay_off_write = replay_off_combine;
          last_replay_ts_write = last_replay_ts_combine;
          ts_cb++;
        }
      }
    }
    while (ts_wrt.load() < ts_cb.load());
    std::swap(tb_combine, tb_write);
    replay_off_write = replay_off_combine;
    last_replay_ts_write = last_replay_ts_combine;
    ts_cb++;
    
    replay_stop = true;
    th_wrt.join();
  }
};

LogFlusher **log_flushers;
LogReplayer *log_replayer;

uint64_t LogFlusher::log_area_size = 0;
uint64_t LogFlusher::log_page_num = 0;
LogFlusher** LogFlusher::flushers = nullptr;

void init_logers() {
  LogFlusher::log_area_size = PSTM_LOG_SIZE / PAGE_SIZE / flusher_count * PAGE_SIZE;
  LogFlusher::log_page_num = LogFlusher::log_area_size / PAGE_SIZE; 
  log_flushers = (LogFlusher **)malloc(sizeof(LogFlusher *) * flusher_count);
  for (int i = 0; i < flusher_count; i ++) {
    if (FLUSHER_TYPE == 0) log_flushers[i] = new LogFlusher(i);
    else if (FLUSHER_TYPE == 1) log_flushers[i] = new CombinedLogFlusher(i);
  }
  LogFlusher::flushers = log_flushers;
  log_replayer = new LogReplayer(log_flushers);

}

void create_log_threads() {
  if (FLUSHER_TYPE != 0) {
    for (int i = 0; i < flusher_count; i++) {
      log_thread_pool.push_back(std::thread(&LogFlusher::do_flush_thread, log_flushers[i]));
    }
  }
  #ifdef ENABLE_REPLAY
  log_thread_pool.push_back(std::thread(&LogReplayer::do_replay_thread, log_replayer));
  #endif
}

void join_log_threads() {
  for (uint64_t i = 0; i < log_thread_pool.size(); i ++) {
    log_thread_pool[i].join();
  }
}

void pstm_plog_commit() {
  if (FLUSHER_TYPE == 0 && thread_vlog_entry->log_count > 0) {
    #ifdef ENABLE_FLUSH
    log_flushers[thread_id]->do_flush_vlog(thread_vlog_entry);
    #endif
    #ifdef PDEBUG
    log_flushers[thread_id]->persist_ts.push_back(thread_vlog_entry->ts);
    #endif
  }
}

// void pstm_plog_init_thread() {
//   if (FLUSHER_TYPE == 0) log_flushers[thread_id]->oldest_no_persist_vlog = thread_vlog_entry;
// }

void pstm_plog_exit_thread() {
  if (FLUSHER_TYPE == 0) log_flushers[thread_id]->log_end_signal.store(true);
}

void pstm_plog_block_read(uint64_t ts) {
  if (FLUSHER_TYPE != 0)
    log_flushers[flusher_id]->block_until_persist(ts);
}

void pstm_plog_init() {
  init_logers();
  for (int i = 0; i < flusher_count; i ++) {
    log_root_t *log_root = log_flushers[i]->log_root_ptr;
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
  create_log_threads();
}

void pstm_plog_end() {
  pstm_stop_signal.store(true);
  join_log_threads();
  for (int i = 0; i < flusher_count; i ++) {
    log_root_t *log_root = log_flushers[i]->log_root_ptr;
  
    log_root->crash = 0;
    FLUSH_CL(&log_root->crash);
    FENCE_PREV_FLUSHES();
  }
}

uint64_t pstm_plog_trace_dep(int thread_id, uint64_t ts) {
  if (log_flushers[thread_id]->last_persist_ts >= ts) return UINT64_MAX;
  else return log_flushers[thread_id]->get_log_offset(ts);
}

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