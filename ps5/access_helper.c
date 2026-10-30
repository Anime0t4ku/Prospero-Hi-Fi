/* Prospero Hi-Fi: one-shot storage access helper, executed only by an existing
 * local PS5 ELF loader. No listener and no persistent kernel patches.
 * Public PS5 payload SDK APIs provide firmware-specific offsets.
 * GPL-3.0-or-later, Anime0t4ku.
 */
#include <ps5/kernel.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
struct Request { char marker[16]; int32_t pid; uint32_t nonce; uint64_t address; };
// The app patches this unique record in a copy of the ELF before sending it.
volatile struct Request prospero_request={{'P','R','O','P','E','R','O','_','A','C','C','E','S','S','!','!'},-1,0,0};
struct Handshake { char marker[16]; int32_t pid; uint32_t nonce; int32_t result; };
int main(void){
    struct Request req;memcpy(&req,(const void*)&prospero_request,sizeof req);
    if(req.pid<1||req.pid==getpid()||req.address==0)return 1;
    struct Handshake state;
    if(kernel_proc_copyout(req.pid,(intptr_t)req.address,&state,sizeof state)!=0)return 2;
    if(memcmp(state.marker,req.marker,16)||state.pid!=req.pid||state.nonce!=req.nonce)return 3;
    intptr_t root=kernel_get_root_vnode(),prison=kernel_get_ucred_prison(0);
    if(!root||!prison)return 4;
    uint8_t caps[16];memset(caps,0xff,sizeof caps);
    int rc=0;
    rc|=kernel_set_ucred_uid(req.pid,0);rc|=kernel_set_ucred_ruid(req.pid,0);rc|=kernel_set_ucred_svuid(req.pid,0);
    rc|=kernel_set_ucred_rgid(req.pid,0);rc|=kernel_set_ucred_svgid(req.pid,0);
    rc|=kernel_set_ucred_prison(req.pid,prison);rc|=kernel_set_ucred_caps(req.pid,caps);
    rc|=kernel_set_proc_rootdir(req.pid,root);rc|=kernel_set_proc_jaildir(req.pid,root);
    state.result=rc==0?1:-1;
    kernel_proc_copyin(req.pid,&state,(intptr_t)req.address,sizeof state);
    return rc!=0;
}
