#ifndef TIME_MEASURE_H
#define TIME_MEASURE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct EventEntry {
  uint64_t event;  // composed with 4bit event id and 60bit tid
  uint64_t ns_count;
} eventEntry;

int time_measure_init(int threadNum);
int time_measure_enter_thread(int threadID);

int time_measure_record(uint64_t event);

int time_measure_delay_imm();
int time_measure_delay_deptrc();
int time_measure_delay_group();

int time_measure_lost_deptrc();
int time_measure_lost_group_commit();

#ifdef __cplusplus
}
#endif

#endif