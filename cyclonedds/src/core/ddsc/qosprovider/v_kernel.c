#include "v_kernel.h"
#include "os_thread.h"

int32_t
v_kernelThreadInfoGetDomainId(void)
{
    // struct v__kernelThreadInfo *threadMemInfo;
    // ////os_threadMemGet
    // threadMemInfo = os_threadMemGet(OS_THREAD_PROCESS_INFO);
    // if (threadMemInfo) {
    //     return (threadMemInfo->serial & V_KERNEL_THREAD_FLAG_DOMAINID);
    // }
    return -1;
}