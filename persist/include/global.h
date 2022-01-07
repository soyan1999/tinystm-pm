#ifndef _GLOBAL_H_
#define _GLOBAL_H_


#include <stdint.h>


#define ARCH_CACHE_LINE_SIZE    64


#ifdef PDEBUG
#define ASSERT(t)               assert(t)
#else
#define ASSERT(t)
#endif



#ifdef __cplusplus
extern "C" {
#endif



#ifdef __cplusplus
}
#endif


#endif