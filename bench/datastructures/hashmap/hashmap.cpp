#include <assert.h>
#include <getopt.h>
#include <limits.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include "timer.h"

#define DEFAULT_DURATION                10000
#define DEFAULT_INITIAL                 256
#define DEFAULT_NB_THREADS              1
#define DEFAULT_RANGE                   0xFFFF
#define DEFAULT_SEED                    0
#define DEFAULT_UPDATE                  20

#define XSTR(s)                         STR(s)
#define STR(s)                          #s

/* ################################################################### *
 * GLOBALS
 * ################################################################### */

#include "tm.h"

__thread void* rot_readset[1024];
__thread char crot_readset[8192];
__thread int irot_readset[2048];
__thread int16_t i2rot_readset[4096];

unsigned int htm_rot_enabled = 1;
unsigned int allow_rots_ros = 1;
unsigned int allow_htms = 1;

__attribute__((aligned(CACHE_LINE_SIZE))) pthread_spinlock_t fallback_in_use;
__attribute__((aligned(CACHE_LINE_SIZE))) padded_scalar_t exists_sw;

__thread unsigned long backoff = MIN_BACKOFF;
__thread unsigned long cm_seed = 123456789UL;

__attribute__((aligned(CACHE_LINE_SIZE))) padded_statistics_t stats_array[80];

__attribute__((aligned(CACHE_LINE_SIZE))) pthread_spinlock_t single_global_lock = 0;

__attribute__((aligned(CACHE_LINE_SIZE))) padded_scalar_t counters[80];

__thread unsigned int local_exec_mode = 0;

__thread unsigned int local_thread_id;

__thread long rs_mask_2 = 0xffffffffffff0000;
__thread long rs_mask_4 = 0xffffffff00000000;
__thread long offset = 0;
__thread char* p;
__thread int* ip;
__thread int16_t* i2p; 
__thread long moffset = 0;
__thread long moffset_2 = 0;
__thread long moffset_6 = 0;

__thread unsigned long rs_counter = 0;

unsigned int allow_stms = 0;

unsigned int ucb_levers = 3;
unsigned long ucb_trials[3];
unsigned long total_trials;
unsigned int selected_lever[50000];

typedef struct Node_HM_t
{
	long m_val;
	struct Node_HM_t* m_next;
} Node_HM;

typedef struct List_t
{
	Node_HM* sentinel;
} List;

static volatile int stop;

static int N_BUCKETS = 512;

List** bucket;

TM_CALLABLE
long hm_insert_htm(TM_ARGDECL List* set, long val)
{
  // traverse the list to find the insertion point
  Node_HM* prev = set->sentinel;
  Node_HM* curr = FAST_PATH_SHARED_READ_P(prev->m_next);
	long temp;
  while (curr != NULL) {
    temp = FAST_PATH_SHARED_READ(curr->m_val);
    if (temp >= val)
      break;
    prev = curr;
    curr = FAST_PATH_SHARED_READ_P(prev->m_next);
  }
	if(curr){
		temp = FAST_PATH_SHARED_READ(curr->m_val);
	}
  if (!curr || (temp > val)) {
    Node_HM* insert_point = (Node_HM*)(prev);

    // create the new node
    Node_HM* i = (Node_HM*)TM_MALLOC(sizeof(Node_HM));
    i->m_val = val;
    i->m_next = (Node_HM*)(curr);
		//int x;
		//for(x = 0; x < 1000 ; x++)
    FAST_PATH_SHARED_WRITE_P(insert_point->m_next, i);
		return 1;
  }
	return 0;
}

void hm_insert_seq(List* set, long val)
{
  Node_HM* prev = set->sentinel;
  Node_HM* curr = prev->m_next;

  while (curr != NULL) {
    if (curr->m_val >= val)
      break;
    prev = curr;
    curr = prev->m_next;
  }

  // now insert new_node between prev and curr
  if (!curr || (curr->m_val > val)) {
    Node_HM* insert_point = (Node_HM*)(prev);

    // create the new node
    Node_HM* i = (Node_HM*)malloc(sizeof(Node_HM));
    i->m_val = val;
    i->m_next = (Node_HM*)(curr);
    insert_point->m_next = i;
  }
}

TM_CALLABLE
long hm_lookup_htm(TM_ARGDECL List* set, long val)
{
	int found = 0;
	const Node_HM* curr = set->sentinel;
	curr = FAST_PATH_SHARED_READ_P(curr->m_next);
	long temp; 
	while (curr != NULL) {
		temp = FAST_PATH_SHARED_READ(curr->m_val);
		if (temp >= val)
			break;
		curr = FAST_PATH_SHARED_READ_P(curr->m_next);
	}
	if(curr != NULL){
		temp = FAST_PATH_SHARED_READ(curr->m_val);
		found = (temp  == val);
	}
	return found;
}

TM_CALLABLE
int hm_remove_htm(TM_ARGDECL List* set, long val)
{
	Node_HM* prev = set->sentinel;
	Node_HM* curr = FAST_PATH_SHARED_READ_P(prev->m_next);
	while (curr != NULL) {
		long temp = FAST_PATH_SHARED_READ(curr->m_val);
		Node_HM* temp_p;
		if (temp == val) {
			Node_HM* mod_point = (Node_HM*)(prev);
			temp_p = FAST_PATH_SHARED_READ_P(curr->m_next);
			FAST_PATH_SHARED_WRITE_P(mod_point->m_next, temp_p);

			FAST_PATH_FREE((Node_HM*)(curr));
			return 1;
		}
		else if (temp > val) {
			return 0;
		}
		prev = curr;
		curr = FAST_PATH_SHARED_READ_P(prev->m_next);
	}
	return 0;
}

TM_CALLABLE
long hm_insert_stm(TM_ARGDECL List* set, long val)
{
  // traverse the list to find the insertion point
  Node_HM* prev = set->sentinel;
  Node_HM* curr = SLOW_PATH_SHARED_READ_P(prev->m_next);
	long temp;
  while (curr != NULL) {
		temp = SLOW_PATH_SHARED_READ(curr->m_val);
    if (temp >= val)
            break;
    prev = curr;
    curr = SLOW_PATH_SHARED_READ_P(prev->m_next);
  }
	if(curr){
		temp = SLOW_PATH_SHARED_READ(curr->m_val);
	}
	if(!curr || (temp > val)) {
    Node_HM* insert_point = (Node_HM*)(prev);

    // create the new node
    Node_HM* i = (Node_HM*)TM_MALLOC(sizeof(Node_HM));
    i->m_val = val;
    i->m_next = (Node_HM*)(curr);
		int red_i;
    SLOW_PATH_SHARED_WRITE_P(insert_point->m_next, i);
		return 1;
	}
	return 0;
}

TM_CALLABLE
long hm_lookup_stm(TM_ARGDECL List* set, long val)
{
	int found = 0;
	Node_HM* prev = set->sentinel;
        Node_HM* curr = SLOW_PATH_SHARED_READ_P(prev->m_next);
	long temp;
	while (curr != NULL) {
		temp = SLOW_PATH_SHARED_READ(curr->m_val);
		if (temp >= val)
			break;
		prev = curr;
		curr = SLOW_PATH_SHARED_READ_P(prev->m_next);
	}
	if(curr != NULL){
		temp = SLOW_PATH_SHARED_READ(curr->m_val);
		found = (temp == val);
	}
	return found;
}

TM_CALLABLE
int hm_remove_stm(TM_ARGDECL List* set, long val)
{
	Node_HM* prev = set->sentinel;
	Node_HM* curr = SLOW_PATH_SHARED_READ_P(prev->m_next);
	while (curr != NULL) {
		long temp = SLOW_PATH_SHARED_READ(curr->m_val);
		Node_HM* temp_p;
		if (temp == val) {
			Node_HM* mod_point = (Node_HM*)(prev);
			temp_p = SLOW_PATH_SHARED_READ_P(curr->m_next);
			SLOW_PATH_SHARED_WRITE_P(mod_point->m_next, temp_p);

			SLOW_PATH_FREE((Node_HM*)(curr));
			return 1;
		}
		else if (temp > val) {
			return 0;
		}
		prev = curr;
		curr = SLOW_PATH_SHARED_READ_P(prev->m_next);
	}
	return 0;
}

TM_CALLABLE
long priv_insert_htm(TM_ARGDECL long val)
{
    return hm_insert_htm(TM_ARG (bucket[val % N_BUCKETS]), val);
}

void priv_insert_seq(long val)
{
	hm_insert_seq( (bucket[val % N_BUCKETS]), val);
}

TM_CALLABLE
long priv_lookup_htm(TM_ARGDECL long val)
{
    return hm_lookup_htm(TM_ARG (bucket[val % N_BUCKETS]), val);
}

TM_CALLABLE
int priv_remove_item_htm(TM_ARGDECL long val)
{
    return hm_remove_htm(TM_ARG (bucket[val % N_BUCKETS]), val);
}

TM_CALLABLE
long priv_insert_stm(TM_ARGDECL long val)
{
    return hm_insert_stm(TM_ARG (bucket[val % N_BUCKETS]), val);
}

TM_CALLABLE
long priv_lookup_stm(TM_ARGDECL long val)
{
    return hm_lookup_stm(TM_ARG (bucket[val % N_BUCKETS]), val);
}

TM_CALLABLE
int priv_remove_item_stm(TM_ARGDECL long val)
{
    return hm_remove_stm(TM_ARG (bucket[val % N_BUCKETS]), val);
}

long set_add_seq(long val) {
	priv_insert_seq(val);
 return 1;
}

long set_add(TM_ARGDECL long val)
{
    int res = 0;
    int ro = 0;
    TM_BEGIN_EXT(0,ro);
    res = (local_exec_mode == 3 || local_exec_mode == 1 || local_exec_mode == 4) ? priv_insert_stm(TM_ARG val) : priv_insert_htm(TM_ARG val);
    TM_END();

    return res;
}

int set_remove(TM_ARGDECL long val)
{
    int res = 0;

    int ro = 0;
    TM_BEGIN_EXT(1,ro);
    res = (local_exec_mode == 3 || local_exec_mode == 1 || local_exec_mode == 4) ? priv_remove_item_stm(TM_ARG val) : priv_remove_item_htm(TM_ARG val);
    TM_END();

    return res;
}

long set_contains(TM_ARGDECL long  val)
{
    long res = 0;

    int ro = 1;
    TM_BEGIN_EXT(2, ro);
    res = (local_exec_mode == 3 || local_exec_mode == 1 || local_exec_mode == 4) ? priv_lookup_stm(TM_ARG val) : priv_lookup_htm(TM_ARG val);
    TM_END();

    return res;
}

#include <sched.h>
  long range;
  int update;
  unsigned long nb_add;
  unsigned long nb_remove;
  unsigned long nb_contains;
  unsigned long nb_found;
  unsigned long nb_aborts;
  unsigned int nb_threads;
  unsigned int seed;
  long operations;

void *test(void *data)
{

  TM_THREAD_ENTER();

  unsigned int mySeed = seed + sched_getcpu();

  long myOps = operations / nb_threads;
  long val = -1;
  int op;

  while (myOps > 0) {
    op = rand_r(&mySeed) % 100;
    if (op < update) {
      if (val == -1) {
        val = (rand_r(&mySeed) % range) + 1;
        if(set_add(TM_ARG val) == 0) {
          val = -1;
        }
      } else {
        set_remove(TM_ARG  val);
        val = -1;
      }
    } else {
      long tmp = (rand_r(&mySeed) % range) + 1;
      set_contains(TM_ARG tmp);
    }

    myOps--;
  }

  TM_THREAD_EXIT();
  return NULL;
}

# define no_argument        0
# define required_argument  1
# define optional_argument  2

MAIN(argc, argv) {
  TIMER_T start;
  TIMER_T stop;

  struct option long_options[] = {
    // These options don't set a flag
    {"help",                      no_argument,       NULL, 'h'},
    {"duration",                  required_argument, NULL, 'd'},
    {"initial-size",              required_argument, NULL, 'i'},
    {"num-threads",               required_argument, NULL, 'n'},
    {"range",                     required_argument, NULL, 'r'},
    {"seed",                      required_argument, NULL, 's'},
    {"buckets",                   required_argument, NULL, 'b'},
    {"update-rate",               required_argument, NULL, 'u'},
    {NULL, 0, NULL, 0}
  };

  int i, c;
  long val;
  operations = DEFAULT_DURATION;
  unsigned int initial = DEFAULT_INITIAL;
  nb_threads = DEFAULT_NB_THREADS;
  range = DEFAULT_RANGE;
  update = DEFAULT_UPDATE;
  N_BUCKETS = 512;

  while(1) {
    i = 0;
    c = getopt_long(argc, argv, "hd:i:n:b:r:s:u:", long_options, &i);

    if(c == -1)
      break;

    if(c == 0 && long_options[i].flag == 0)
      c = long_options[i].val;

    switch(c) {
     case 0:
       /* Flag is automatically set */
       break;
     case 'h':
       printf("intset -- STM stress test "
              "(hash map)\n"
              "\n"
              "Usage:\n"
              "  intset [options...]\n"
              "\n"
              "Options:\n"
              "  -h, --help\n"
              "        Print this message\n"
              "  -d, --duration <int>\n"
              "        Test duration in milliseconds (0=infinite, default=" XSTR(DEFAULT_DURATION) ")\n"
              "  -i, --initial-size <int>\n"
              "        Number of elements to insert before test (default=" XSTR(DEFAULT_INITIAL) ")\n"
              "  -n, --num-threads <int>\n"
              "        Number of threads (default=" XSTR(DEFAULT_NB_THREADS) ")\n"
              "  -r, --range <int>\n"
              "        Range of integer values inserted in set (default=" XSTR(DEFAULT_RANGE) ")\n"
              "  -s, --seed <int>\n"
              "        RNG seed (0=time-based, default=" XSTR(DEFAULT_SEED) ")\n"
              "  -u, --update-rate <int>\n"
              "        Percentage of update transactions (default=" XSTR(DEFAULT_UPDATE) ")\n"
         );
       exit(0);
     case 'd':
       operations = atoi(optarg);
       break;
     case 'b':
       N_BUCKETS = atoi(optarg);
       break;
     case 'i':
       initial = atoi(optarg);
       break;
     case 'n':
       nb_threads = atoi(optarg);
       break;
     case 'r':
       range = atoi(optarg);
       break;
     case 's':
       seed = atoi(optarg);
       break;
     case 'u':
       update = atoi(optarg);
       break;
     case '?':
       printf("Use -h or --help for help\n");
       exit(0);
     default:
       exit(1);
    }
  }

  if (seed == 0)
    srand((int)time(0));
  else
    srand(seed);

  SIM_GET_NUM_CPU(nb_threads);
  TM_STARTUP(nb_threads);
  P_MEMORY_STARTUP(nb_threads);
  thread_startup(nb_threads);

  bucket = (List**) malloc(N_BUCKETS*sizeof(List*));

  for (i = 0; i < N_BUCKETS; i++) {
    bucket[i] = (List*) malloc (sizeof(List));
    bucket[i]->sentinel = (Node_HM*) malloc(sizeof(Node_HM));
    bucket[i]->sentinel->m_val = LONG_MIN;
    bucket[i]->sentinel->m_next = NULL;
  }

  /* Populate set */
  printf("Adding %d entries to set\n", initial);
  for (i = 0; i < initial; i++) {
    val = (rand() % range) + 1;
    set_add_seq(val);
  }
  puts("Added\n");
  seed = rand();
  TIMER_READ(start);
  GOTO_SIM();
//startEnergyIntel();

 thread_start(test, NULL);

  GOTO_REAL();
  TIMER_READ(stop);

//double energy = endEnergyIntel();
  puts("done.");
  stats_benchTime = TIMER_DIFF_SECONDS(start, stop);
  printf("\nTime = %0.6lf\n", stats_benchTime);
//printf("Energy = %0.6lf\n", energy);
  fflush(stdout);

  TM_SHUTDOWN();
  P_MEMORY_SHUTDOWN();
  GOTO_SIM();
  thread_shutdown();
  MAIN_RETURN(0);
}
