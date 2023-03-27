#ifndef V_KERNEL_H
#define V_KERNEL_H

#if defined (__cplusplus)
extern "C" {
#endif


#include "c_typebase.h"
#include "c_mmbase.h"
#include "os_retcode.h"

#define V_KERNEL_THREAD_FLAG_DOMAINID       (0xFF)


#define v_processInfo(p) (C_CAST(p,v_processInfo))
#define v_threadInfo(p) (C_CAST(p,v_threadInfo))
#define OS_UNUSED_ARG(a) (void) (a)



typedef enum {
    V_RESULT_UNDEFINED = OS_RETCODE_ID_V_RESULT,
    V_RESULT_OK,
    V_RESULT_INTERRUPTED,
    V_RESULT_NOT_ENABLED,
    V_RESULT_OUT_OF_MEMORY,
    V_RESULT_INTERNAL_ERROR,
    V_RESULT_ILL_PARAM,
    V_RESULT_CLASS_MISMATCH,
    V_RESULT_DETACHING,
    V_RESULT_TIMEOUT,
    V_RESULT_OUT_OF_RESOURCES,
    V_RESULT_INCONSISTENT_QOS,
    V_RESULT_IMMUTABLE_POLICY,
    V_RESULT_PRECONDITION_NOT_MET,
    V_RESULT_ALREADY_DELETED,
    V_RESULT_HANDLE_EXPIRED,
    V_RESULT_NO_DATA,
    V_RESULT_UNSUPPORTED
} v_result;

struct v__kernelThreadInfo {
    /* Counts the number of times this thread has invoked v_kernelProtect for
     * this domain. */
    uint32_t myProtectCount;
    /* Maintains the state of this thread. */
    uint32_t flags;
    /* The process specific record in SHM */
    ////
    //v_processInfo pinfo;
    /* The thread specific record in SHM */
    //v_threadInfo tinfo;
    /* Domain-ID + serial that pinfo (and thereby tinfo) corresponds to; needed
     * since we can't always dereference pinfo (in potentially detached SHM).
     * The pointer to pinfo may be reused as well. This is a caching mechanism
     * for the last accessed domain. Otherwise every protect needs to acquire
     * process-wide mutex to lookup the threadInfo-record. */
    c_ulong serial;
    /* Pointer to flag that, when non-zero, indicates that threads should block
     * on waking up from sleeps. */
    ddsrt_atomic_uint32_t *block;
    uint32_t blockmask;
    /* Process-private mutex used to deadlock a thread waiting in the kernel so
     * that the SHM can be unmapped without causing SIGSEGV's. */
    os_mutex *deadlock;
    /* Pointer to userdata returned on last v_kernelUnprotect(). */
    void *usrData;
};


int32_t v_kernelThreadInfoGetDomainId(void);

#if defined (__cplusplus)
}
#endif

#endif /* V_KERNEL_H */
