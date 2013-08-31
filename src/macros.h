#ifndef _MACROS_H_
#define _MACROS_H_

//#define ENABLE_TRACE

//#define inline __attribute__ ((noinline))

#ifdef ENABLE_TRACE
#define print(...) printf(__VA_ARGS__)
#else
#define print(...) (void)0
#endif

#endif // _MACROS_H_
