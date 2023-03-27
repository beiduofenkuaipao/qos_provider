#ifndef OS_THREAD_H
#define OS_THREAD_H


#include "c_typebase.h"

#if defined (__cplusplus)
extern "C" {
#endif

typedef pthread_t os_threadId;
typedef pthread_cond_t os_cond;

typedef enum os_threadMemoryIndex {
    OS_THREAD_CLOCK_OFFSET,
    OS_THREAD_UT_TRACE,
    OS_THREAD_JVM,
    OS_THREAD_PROTECT,
    OS_THREAD_API_INFO,
    OS_THREAD_WARNING,
    OS_THREAD_ALLOCATOR_STATE,
    OS_THREAD_NAME,
    OS_THREAD_REPORT_STACK,
    OS_THREAD_PROCESS_INFO,
    OS_THREAD_STATE, /* Used for monitoring thread progress */
    OS_THREAD_STR_ERROR,
    OS_THREAD_MEM_ARRAY_SIZE /* Number of slots in Thread Private Memory */
} os_threadMemoryIndex;






#if defined (__cplusplus)
}
#endif

#endif /* OS_THREAD_H */