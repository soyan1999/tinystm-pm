#ifndef TM_H
#define TM_H 1

#  include <stdio.h>

#include "pmdk.h"

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

#  define P_MEMORY_STARTUP(numThread)   pobj_before_tm_start();//pstm_nb_threads = numThread;unsigned long exec_commits, exec_aborts;
#  define P_MEMORY_SHUTDOWN()           pobj_after_tm_end();\
  // printf("#" \
  //   "THREADS\t"       \
  //   "TIME\t"          \
  //   "COMMITS\t"       \
  //   "ABORTS\n"        \
  // ); \
  // printf("%i\t", pstm_nb_threads); \
  // printf("%f\t", stats_benchTime); \
  // printf("%li\t", exec_commits); \
  // printf("%li\n", exec_aborts); \

#  include <assert.h>
#  include "memory.h"
#  include "types.h"
#  include "thread.h"
#  include <math.h>

// #  include <stm.h>
// #  include <wrappers.h>
// #  include <mod_mem.h>
// #  include <mod_stats.h>
#include "threading.h"

// typedef struct stm_tx stm_tx_t;

static int pstm_nb_threads;

#  define TM_ARG                         /* nothing */
#  define TM_ARG_ALONE                   /* nothing */
#  define TM_ARGDECL                     /* nothing */
#  define TM_ARGDECL_ALONE               /* nothing */
#  define TM_CALLABLE                    /* nothing */

#  define TM_BEGIN_WAIVER()
#  define TM_END_WAIVER()

// outside the TX
# define S_MALLOC(_size)                pobj_malloc(_size)
# define S_FREE(ptr)                    pobj_free(ptr)

# define P_MALLOC(_size)                pobj_malloc(_size);
# define P_MALLOC_THR(_size, _thr)      pobj_malloc(_size);
# define P_FREE(ptr)                    pobj_free(ptr)

// inside the TX
// TODO: cannot write twice to the same memory location
# define TM_MALLOC(_size)               pobj_tx_malloc(_size);
# define TM_FREE(ptr)                   pobj_tx_free(ptr)

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

# define TM_STARTUP(numThread)         pobj_before_tm_start()

# define TM_SHUTDOWN()                 pobj_after_tm_end()


#ifndef NPROFILE /* do profile */
#  define TM_THREAD_EXIT_PROFILE() \
  if (state_profile != NULL) state_profile(thread_getId());
#else /* NPROFILE */
#  define TM_THREAD_EXIT_PROFILE() /* empty */
#endif /* end NPROFILE */

# define TM_THREAD_ENTER()

# define TM_THREAD_EXIT()

# define IS_LOCKED(lock)              *((volatile int*)(&lock)) != 0

# define TM_BEGIN()                   TX_BEGIN(pool_){
//

# define TM_END()                     }TX_END
//

# define TM_RESTART()                  
# define TM_EARLY_RELEASE(var)

# define TM_LOCAL_WRITE(var, val)      ({ var = val; var; })
# define TM_LOCAL_WRITE_P(var, val)    ({ var = val; var; })
# define TM_LOCAL_WRITE_D(var, val)    ({ var = val; var; })

# define TM_SHARED_READ(var)           (var)
// # define TM_SHARED_READ_P(var)       ((__typeof__(var)) stm_load_ptr((volatile void **)(void *)&(var)))
# define TM_SHARED_READ_P(var)         (var)

// extern __thread struct stm_tx * thread_tx;
// # define TM_SHARED_READ_P(var)       ({ \
//   stm_tx_t *tx = thread_tx; \
//   union { stm_word_t w; void *v; } convert; \
//   convert.w = TM_LOAD((stm_word_t *)&(var)); \
//   convert.v; \
// })


# define TM_SHARED_READ_D(var)       (var)
# define TM_SHARED_READ_F(var)       (var)
# define TM_SHARED_WRITE(var, val)   {pobj_before_store(&var,sizeof(var));var=val;}
# define TM_SHARED_WRITE_P(var, val) {pobj_before_store(&var,sizeof(var));var=val;}
# define TM_SHARED_WRITE_D(var, val) {pobj_before_store(&var,sizeof(var));var=val;}
# define TM_SHARED_WRITE_F(var, val) {pobj_before_store(&var,sizeof(var));var=val;}

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
