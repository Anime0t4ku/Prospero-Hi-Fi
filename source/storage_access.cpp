#include <cstdint>
#include <cstring>
#include <fstream>
#include <vector>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <poll.h>
#include <fcntl.h>
#include <chrono>
#include <thread>
namespace nxh {
bool ensureStorageAccess(){
#ifdef __PROSPERO__
    // A sandboxed title can see /app0 but not /data. Ask the already-running
    // localhost ELF loader to run our bundled one-shot helper for this PID.
    if(geteuid()==0 && access("/data",W_OK)==0)return true;
    std::ifstream input("/app0/assets/access-helper.elf",std::ios::binary);
    std::vector<char> elf((std::istreambuf_iterator<char>(input)),{});
    constexpr char marker[17]="PROPERO_ACCESS!!";
    static struct Handshake {char marker[16];int32_t pid;uint32_t nonce;int32_t result;} state;
    std::memcpy(state.marker,marker,16);state.pid=getpid();
    state.nonce=uint32_t(std::chrono::steady_clock::now().time_since_epoch().count());state.result=0;
    bool found=false;
    for(size_t i=0;i+32<=elf.size();i++)if(!std::memcmp(elf.data()+i,marker,16)){
        int32_t sentinel=0;std::memcpy(&sentinel,elf.data()+i+16,4);if(sentinel!=-1)continue;
        std::memcpy(elf.data()+i+16,&state.pid,4);std::memcpy(elf.data()+i+20,&state.nonce,4);
        uint64_t address=reinterpret_cast<uintptr_t>(&state);std::memcpy(elf.data()+i+24,&address,8);found=true;break;
    }
    if(!found)return false;
    int fd=socket(AF_INET,SOCK_STREAM,0);if(fd<0)return false;
#ifdef SO_NOSIGPIPE
    int noSignal=1;setsockopt(fd,SOL_SOCKET,SO_NOSIGPIPE,&noSignal,sizeof noSignal);
#endif
    sockaddr_in endpoint{};endpoint.sin_len=sizeof endpoint;endpoint.sin_family=AF_INET;endpoint.sin_port=htons(9021);endpoint.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    fcntl(fd,F_SETFL,fcntl(fd,F_GETFL,0)|O_NONBLOCK);
    connect(fd,(sockaddr*)&endpoint,sizeof endpoint);pollfd waiter{fd,POLLOUT,0};
    int error=0;socklen_t size=sizeof error;
    if(poll(&waiter,1,1000)<=0||getsockopt(fd,SOL_SOCKET,SO_ERROR,&error,&size)<0||error){close(fd);return false;}
    auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
    size_t sent=0;while(sent<elf.size()&&std::chrono::steady_clock::now()<deadline){
        if(poll(&waiter,1,100)<=0)continue;
        auto n=send(fd,elf.data()+sent,elf.size()-sent,0);if(n>0)sent+=n;
    }
    shutdown(fd,SHUT_WR);close(fd);
    if(sent!=elf.size())return false;
    // Volatile observation: the loader writes our handshake through process memory.
    for(int n=0;n<100;n++){
        int result=*reinterpret_cast<volatile int32_t*>(&state.result);
        if(result)return result==1&&access("/data",W_OK)==0;
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    return false;
#else
    return true;
#endif
}
}
