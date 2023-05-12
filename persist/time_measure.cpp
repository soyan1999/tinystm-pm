#include <bits/stdc++.h>
#include "rdtsc.h"
#include "time_measure.h"

#define COUNT_SUM 40000000UL
#define CRASH_CNT 100
// #define MEASURE_FLUSH

using namespace std;

int thread_num;
uint64_t init_time_point;
static thread_local int thread_id = -1;
EventEntry **buffers;
uint64_t *counters;



int time_measure_init(int threadNum) {
  thread_num = threadNum;

  counters = (uint64_t *)malloc(sizeof(uint64_t *) * thread_num);
  fill(counters, counters+thread_num, 0);

  buffers = (EventEntry **)malloc(sizeof(EventEntry *) * thread_num);
  for (int i = 0; i < thread_num; i ++) {
    buffers[i] = (EventEntry *)malloc(sizeof(EventEntry) * COUNT_SUM/thread_num);
  }

  init_time_point = rdtscp();

  return 0;
}

int time_measure_enter_thread(int threadID) {
  thread_id = threadID;
  return 0;
}

int time_measure_record(uint64_t event) {
  assert(thread_id >= 0);
  assert(counters[thread_id] < COUNT_SUM/thread_num);

  auto duration = rdtscp() - init_time_point;

  buffers[thread_id][counters[thread_id]].event = event;
  buffers[thread_id][counters[thread_id]++].ns_count = duration;

  return 0;
}

int time_measure_delay_imm() {
  std::ofstream fs_delay_commit("time_measure_0.txt", std::ios::out);
  std::ofstream fs_delay_wait("time_measure_1.txt", std::ios::out);
  for (int i = 0; i < thread_num; i ++) {
    uint64_t cnt = 0;
    assert(counters[i]%3 == 0);
    while (cnt < counters[i]) {
      assert(buffers[i][cnt].event == 0);
      uint64_t ts0 = buffers[i][cnt++].ns_count;
      assert(buffers[i][cnt].event == 1);
      uint64_t ts1 = buffers[i][cnt++].ns_count;
      assert(buffers[i][cnt].event == 2);
      uint64_t ts2 = buffers[i][cnt++].ns_count;

      fs_delay_commit << ts2-ts0 << std::endl;
      fs_delay_wait << ts2-ts1 << std::endl;
    }
  }

  fs_delay_commit.close();
  fs_delay_wait.close();
  return 0;
}

int time_measure_lost_deptrc() {
  uint64_t lost_sum = 0, sum_cnt = 0;
  default_random_engine seed(time(nullptr));
  vector<uint64_t> time_crashed;
  uint64_t last_commit_time = UINT64_MAX, begin_time = 0;
  for (int i = 0; i < thread_num; i ++) {
    last_commit_time = min(last_commit_time, buffers[i][counters[i]-1].ns_count);
    begin_time = max(begin_time, buffers[i][0].ns_count);
  }
  assert(last_commit_time > begin_time);
  uniform_int_distribution<uint64_t> ge(begin_time, last_commit_time);
  for (int i = 0; i < CRASH_CNT; i ++) {
    time_crashed.push_back(ge(seed));
  }
  sort(time_crashed.begin(), time_crashed.end(), std::greater<uint64_t>());

  vector<uint64_t> indexs(thread_num);
  while (!time_crashed.empty()) {
    int drop_cnt = 0;
    for (int i = 0; i < thread_num; i ++) {
      while (indexs[i] < counters[i] && buffers[i][indexs[i]].ns_count < time_crashed.back()) indexs[i] ++;

      if (indexs[i] == counters[i]) break;
      for (uint64_t j = indexs[i]; j > 0 && buffers[i][j].event != 1; j --) drop_cnt ++;
    }
    
    lost_sum += drop_cnt;
    sum_cnt ++;
    time_crashed.pop_back();
  }

  printf("drop_count:\t%lf\n", (double)lost_sum/(double)(sum_cnt));
  return 0;
}

int time_measure_lost_group_commit() {
  uint64_t lost_sum = 0, sum_cnt = 0;
  default_random_engine seed(time(nullptr));
  vector<uint64_t> time_crashed;
  uint64_t last_commit_time = UINT64_MAX, begin_time = 0;
  for (int i = 0; i < thread_num; i ++) {
    last_commit_time = min(last_commit_time, buffers[i][counters[i]-1].ns_count);
    begin_time = max(begin_time, buffers[i][0].ns_count);
  }
  assert(last_commit_time > begin_time);
  uniform_int_distribution<uint64_t> ge(begin_time, last_commit_time);
  for (int i = 0; i < CRASH_CNT; i ++) {
    time_crashed.push_back(ge(seed));
  }
  sort(time_crashed.begin(), time_crashed.end(), std::greater<uint64_t>());

  vector<uint64_t> indexs(thread_num);
  while (!time_crashed.empty()) {
    int drop_cnt = 0;
    for (int i = 0; i < thread_num; i ++) {
      while (indexs[i] < counters[i] && buffers[i][indexs[i]].ns_count < time_crashed.back()) indexs[i] ++;
    }

    vector<uint64_t> commit_indexs = indexs;
    uint64_t top_ts = UINT64_MAX;
    for (int i = 0; i < thread_num; i ++) {
      while (commit_indexs[i] > 0 && buffers[i][commit_indexs[i]].event != 1 ) commit_indexs[i] --;
      top_ts = min(top_ts, buffers[i][commit_indexs[i]].ns_count);
    }

    for (int i = 0; i < thread_num; i ++) {
      for (uint64_t j = indexs[i]; j > 0; j --) {
        if (buffers[i][j].event == 1 && buffers[i][j].ns_count <= top_ts) break;
        if (buffers[i][j].event == 0) drop_cnt ++;
      }
    }

    lost_sum += drop_cnt;
    sum_cnt ++;
    time_crashed.pop_back();
  }

  printf("drop_count:\t%lf\n", (double)lost_sum/(double)(sum_cnt));
  return 0;
}

int time_measure_delay_deptrc() {
  uint64_t delay_sum = 0, sum_cnt = 0;
  for (int i = 0; i < thread_num; i ++) {
    uint64_t cnt = 0;
    
    while (cnt < counters[i]) {
      while (cnt < counters[i] && buffers[i][cnt].event != 1) cnt ++;
      if (cnt == counters[i] && buffers[i][cnt].event != 1) break;

      uint64_t ts_group_commit = buffers[i][cnt].ns_count;

      #ifdef MEASURE_FLUSH
      fs_delay_commit << ts_group_commit-buffers[i][cnt - 1].ns_count << std::endl;
      #else
      for (int64_t j = cnt - 1; j >= 0 && buffers[i][j].event != 1; j --) {
        delay_sum += ts_group_commit-buffers[i][j].ns_count;
        sum_cnt ++;
      }
      #endif

      cnt ++;
    }
  }

  printf("delay_count:\t%lf\n", (double)delay_sum/(double)(sum_cnt));
  return 0;
}

int time_measure_delay_group() {
  uint64_t delay_sum = 0, sum_cnt = 0;
  uint64_t ts_max = 0;
  vector<uint64_t> commit_index(thread_num);
  priority_queue<pair<uint64_t,int>,vector<pair<uint64_t,int>>,greater<pair<uint64_t,int>>> qu;
  for (int i = 0; i < thread_num; i ++) {
    uint64_t j = 0;
    while (buffers[i][j].event != 1) j ++;
    ts_max = max(ts_max, buffers[i][j].ns_count);
    commit_index[i] = j;
    qu.push({buffers[i][j].ns_count, i});
  } 

  while (!qu.empty()) {
    int th_id = qu.top().second;
    for (int64_t i = commit_index[th_id] - 1; i >= 0 && buffers[th_id][i].event != 1; i --) {
      delay_sum += ts_max - buffers[th_id][i].ns_count;
      sum_cnt ++;
    }

    qu.pop();
    commit_index[th_id] ++;
    if (commit_index[th_id] < counters[th_id]) {
      while (commit_index[th_id] < counters[th_id] && buffers[th_id][commit_index[th_id]].event != 1) {
        commit_index[th_id] ++;
      }
      
      if (commit_index[th_id] < counters[th_id] && buffers[th_id][commit_index[th_id]].event == 1) {
        qu.push({buffers[th_id][commit_index[th_id]].ns_count, th_id});
        ts_max = max(ts_max, buffers[th_id][commit_index[th_id]].ns_count);
      }
    }
  }

  printf("delay_count:\t%lf\n", (double)delay_sum/(double)(sum_cnt));
  return 0;
}

int time_measure_exit(char *data_root) {
  for (int i = 0; i < thread_num; i ++) {
    std::fstream fs("time_measure_" + std::to_string(i) + ".txt", std::ios::out);
    for (uint64_t k = 0; k < counters[i]; k ++) {
      fs << buffers[i][k].event << " " << buffers[i][k].ns_count << std::endl;
    }
    fs.close();
  }
  return 0;
}