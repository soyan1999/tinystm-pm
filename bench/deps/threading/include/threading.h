#ifndef THREADING_H_GUARD
#define THREADING_H_GUARD

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

typedef int threading_id;
typedef void(*threading_callback)(threading_id, int nbThreads, void *args);
typedef void(*threading_async_callback)(void*, threading_id asyncId);

// TODO: port __GNUC_ATOMICS notation to other compilers
// namely: __ATOMIC_RELEASE and __ATOMIC_ACQUIRE memory fences
//         __atomic_load_n and __atomic_store_n
//         __sync_* functions

// starts nbThreads+asyncs threads, nbThreads execute the threading_callback
void threading_start(int nbThreads, int asyncs, threading_callback, void *args);
void threading_join();

int threading_getMaximumHardwareThreads();
void threading_pinThisThread(int coreID);
threading_id threading_getCoreID(int coreID);
int threading_getNbThreads();
threading_id threading_getMyThreadID();
int threading_async(int idx, threading_async_callback fn, void *args);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* THREADING_H_GUARD */
