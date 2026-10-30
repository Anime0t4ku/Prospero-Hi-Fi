// POSIX host ID is optional on PS5. Query it when the kernel exposes it.
#include <sys/types.h>
#include <sys/sysctl.h>
long gethostid(void) {
    int mib[2]={CTL_KERN,KERN_HOSTID};long value=0;size_t size=sizeof value;
    if(sysctl(mib,2,&value,&size,0,0)!=0)return 0;
    return value;
}
