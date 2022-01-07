#ifndef TM_H
#define TM_H 1

#  include <stdio.h>

#include "persist.h"

#ifndef REDUCED_TM_API

#  define MAIN(argc, argv)              int main (int argc, char** argv)
#  define MAIN_RETURN(val)              return val

#  define GOTO_SIM()                    /* nothing */
#  define GOTO_REAL()                   /* nothing */
#  define IS_IN_SIM()                   (0)

#  define SIM_GET_NUM_CPU(var)          /* nothing */

#  define TM_PRINTF                     printf
#  define TM_PRINT0                     printf
#  define TM_PRINT1                     printf
#  define TM_PRINT2                     printf
#  define TM_PRINT3                     printf

#  define P_MEMORY_STARTUP(numThread)   pstm_before_tm_start(numThread)
#  define P_MEMORY_SHUTDOWN()           pstm_after_tm_exit()

#  include <assert.h>
#  include "memory.h"
#  include "types.h"
#  include "thread.h"
#  include <math.h>

#  include <stm.h>
#  include <wrappers.h>
#  include <mod_mem.h>
#  include <mod_stats.h>
#include "threading.h"

typedef struct stm_tx stm_tx_t;

static int pstm_nb_threads;

#  define TM_ARG                         /* nothing */
#  define TM_ARG_ALONE                   /* nothing */
#  define TM_ARGDECL                     /* nothing */
#  define TM_ARGDECL_ALONE               /* nothing */
#  define TM_CALLABLE                    /* nothing */

#  define TM_BEGIN_WAIVER()
#  define TM_END_WAIVER()

// outside the TX
# define S_MALLOC(_size)                ({ void *_PTR = pstm_nvmalloc(_size); PADDR_TO_VADDR(_PTR); })
# define S_FREE(ptr)                    /* empty */

# define P_MALLOC(_size)                ({ void *_PTR = pstm_local_nvmalloc(thread_getId(), _size); PADDR_TO_VADDR(_PTR); })
# define P_MALLOC_THR(_size, _thr)      ({ void *_PTR = pstm_local_nvmalloc(_thr, _size); PADDR_TO_VADDR(_PTR); })
# define P_FREE(ptr)                    /* empty */

// inside the TX
// TODO: cannot write twice to the same memory location
# define TM_MALLOC(_size)               ({ void *_PTR = pstm_local_nvmalloc(thread_getId(), _size); PADDR_TO_VADDR(_PTR); })
# define TM_FREE(ptr)                   /* TODO */

# define SETUP_NUMBER_TASKS(n)
# define SETUP_NUMBER_THREADS(n)
# define PRINT_STATS()
# define AL_LOCK(idx)

#endif

#ifdef REDUCED_TM_API
#  define SPECIAL_THREAD_ID()          get_tid()
#else
#  define SPECIAL_THREAD_ID()          thread_getId()
#endif

# define TM_STARTUP(numThread) \
  if (sizeof(long) != sizeof(void *)) { \
    fprintf(stderr, "Error: unsupported long and pointer sizes\n"); \
    exit(1); \
  } \
  stm_init(); \
  mod_mem_init(0); \
  mod_stats_init(); \
  pstm_nb_threads = numThread; \
//

# define TM_SHUTDOWN() \
  unsigned long exec_commits, exec_aborts; \
  stm_get_global_stats("global_nb_commits", &exec_commits); \
  stm_get_global_stats("global_nb_aborts", &exec_aborts); \
  printf("#" \
    "THREADS\t"       \
    "TIME\t"          \
    "COMMITS\t"       \
    "ABORTS\n"        \
  ); \
  printf("%i\t", pstm_nb_threads); \
  printf("%f\t", stats_benchTime); \
  printf("%li\t", exec_commits); \
  printf("%li\n", exec_aborts); \
  stm_exit() \
//


#ifndef NPROFILE /* do profile */
#  define TM_THREAD_EXIT_PROFILE() \
  if (state_profile != NULL) state_profile(thread_getId());
#else /* NPROFILE */
#  define TM_THREAD_EXIT_PROFILE() /* empty */
#endif /* end NPROFILE */

# define TM_THREAD_ENTER() \
  {stm_init_thread();pstm_after_thread_start();} \
//

# define TM_THREAD_EXIT() \
  {pstm_before_thread_exit();stm_exit_thread();} \
//

# define IS_LOCKED(lock)        *((volatile int*)(&lock)) != 0

# define TM_BEGIN() \
do { \
  stm_tx_attr_t _a = {}; \
  sigjmp_buf *buf = stm_start(_a); \
  sigsetjmp((__jmp_buf_tag*)buf, 0); \
} while (0) \
//

# define TM_END() \
  {int ts = stm_commit();pstm_after_tx_commit(ts);!!ts} \
//

# define TM_RESTART()                  stm_abort(0)
# define TM_EARLY_RELEASE(var)

# define TM_LOCAL_WRITE(var, val)      ({ var = val; var; })
# define TM_LOCAL_WRITE_P(var, val)    ({ var = val; var; })
# define TM_LOCAL_WRITE_D(var, val)    ({ var = val; var; })

# define TM_SHARED_READ(var)         stm_load((volatile stm_word_t *)(void *)&(var))
// # define TM_SHARED_READ_P(var)       ((__typeof__(var)) stm_load_ptr((volatile void **)(void *)&(var)))
# define TM_SHARED_READ_P(var)       ((__typeof__(var))stm_load((volatile long *)(void *)&(var)))

// extern __thread struct stm_tx * thread_tx;
// # define TM_SHARED_READ_P(var)       ({ \
//   stm_tx_t *tx = thread_tx; \
//   union { stm_word_t w; void *v; } convert; \
//   convert.w = TM_LOAD((stm_word_t *)&(var)); \
//   convert.v; \
// })


# define TM_SHARED_READ_D(var)       stm_load_double((volatile double *)(void *)&(var))
# define TM_SHARED_READ_F(var)       stm_load_float((volatile float *)(void *)&(var))
# define TM_SHARED_WRITE(var, val)   stm_store((volatile stm_word_t *)(void *)&(var), (stm_word_t)val)
# define TM_SHARED_WRITE_P(var, val) stm_store_ptr((volatile void **)(void *)&(var), val)
# define TM_SHARED_WRITE_D(var, val) stm_store_double((volatile double *)(void *)&(var), val)
# define TM_SHARED_WRITE_F(var, val) stm_store_float((volatile float *)(void *)&(var), val)

// ----------------------------------------------
#define SLOW_PATH_SHARED_READ TM_SHARED_READ
#define FAST_PATH_SHARED_READ TM_SHARED_READ
#define SLOW_PATH_SHARED_READ_D TM_SHARED_READ_D
#define FAST_PATH_SHARED_READ_D TM_SHARED_READ_D
#define SLOW_PATH_SHARED_READ_P TM_SHARED_READ_P
#define FAST_PATH_SHARED_READ_P TM_SHARED_READ_P

#define SLOW_PATH_SHARED_WRITE TM_SHARED_WRITE
#define FAST_PATH_SHARED_WRITE TM_SHARED_WRITE
#define SLOW_PATH_SHARED_WRITE_D TM_SHARED_WRITE_D
#define FAST_PATH_SHARED_WRITE_D TM_SHARED_WRITE_D
#define SLOW_PATH_SHARED_WRITE_P TM_SHARED_WRITE_P
#define FAST_PATH_SHARED_WRITE_P TM_SHARED_WRITE_P
// ----------------------------------------------

extern void(*state_profile)(int);
extern void(*state_print_profile)(char*);
extern char PROFILE_FILE[1024];
extern char ERROR_FILE[1024];
extern int FLUSH_LAT;
extern int numReplayers;
extern int PINNING; // 0 == default, 1 == Fill CPU/SMT/NUMA, 2 == SMT/CPU/NUMA
extern int *PINNING_MAT;

extern long stats_nbSuccess;
extern long stats_nbFallback;
extern long stats_nbAbort;
extern long stats_nbConfl;
extern long stats_nbCapac;
extern long stats_nbExpli;
extern long stats_nbOther;
extern double stats_benchTime;

// extern __thread long stats_nbTXwrites;

#endif
