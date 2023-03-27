#ifndef OS_MUTEX_H
#define OS_MUTEX_H


#if defined (__cplusplus)
extern "C" {
#endif

#include "c_typebase.h"
#include "os_thread.h"

typedef pthread_mutex_t os_mutex;
typedef pthread_rwlock_t os_rwlock;
typedef pthread_cond_t c_cond;

typedef struct {
   os_threadId owner;
   os_mutex mtx;
} c_mutex;

typedef struct c_threadId {
    os_threadId value;
} c_threadId;


#ifdef NDEBUG
typedef os_rwlock c_lock;
#else
typedef struct {
    os_threadId owner;
    os_rwlock lck;
} c_lock;
#endif






#if defined (__cplusplus)
}
#endif

#endif /* OS_MUTEX_H */
