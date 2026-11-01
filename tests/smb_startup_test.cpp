#include "player.hpp"
#include "extended_audio.hpp"
#include "disc.hpp"
#include <smb2/smb2.h>
#include <smb2/libsmb2.h>
#include <fstream>
#include <set>
#include <cassert>
#include <iostream>
using namespace nxh;
static std::vector<unsigned char> media;static std::atomic<int> connections{0};static std::atomic<uint32_t> firstRead{0};static std::mutex handlesMutex;static std::set<smb2_context*> handles;
extern "C" int __wrap_smb2_connect_share(smb2_context*,const char*,const char*,const char*){++connections;SDL_Delay(100);return 0;}
extern "C" smb2fh *__wrap_smb2_open(smb2_context *context,const char*,int){std::lock_guard<std::mutex> lock(handlesMutex);assert(handles.insert(context).second);return reinterpret_cast<smb2fh*>(1);}
extern "C" int __wrap_smb2_fstat(smb2_context*,smb2fh*,smb2_stat_64 *stat){*stat={};stat->smb2_size=media.size();return 0;}
extern "C" int __wrap_smb2_pread(smb2_context*,smb2fh*,uint8_t *out,uint32_t size,uint64_t offset){uint32_t expected=0;firstRead.compare_exchange_strong(expected,size);SDL_Delay(20);size=uint32_t(std::min<uint64_t>(size,offset<media.size()?media.size()-offset:0));if(size)std::copy_n(media.data()+offset,size,out);return int(size);}
extern "C" int __wrap_smb2_close(smb2_context *context,smb2fh*){std::lock_guard<std::mutex> lock(handlesMutex);assert(handles.erase(context)==1);return 0;}
int main(int argc,char **argv){assert(argc==2);std::ifstream file(argv[1],std::ios::binary);media.assign(std::istreambuf_iterator<char>(file),{});assert(media.size()>16384);SDL_setenv("SDL_AUDIODRIVER","dummy",1);assert(!SDL_Init(SDL_INIT_AUDIO|SDL_INIT_TIMER));Share share;share.server="fixture.invalid";share.share="music";Source source{true,share,""};std::string error;unsigned char header[12];
 for(int i=0;i<2;i++){auto input=openInput(source,"track.flac",error);assert(input&&input->read(header,12)==12);assert(connections==1&&firstRead==16384);}
 std::vector<std::thread> concurrent;for(int i=0;i<8;i++)concurrent.emplace_back([&]{std::string error;auto input=openInput(source,"track.flac",error);unsigned char header[12];assert(input&&input->read(header,12)==12);SDL_Delay(30);});for(auto &thread:concurrent)thread.join();assert(handles.empty());
 {Settings settings;settings.gaplessPlayback=true;Player player(settings);auto began=SDL_GetTicks64();player.play({{source,"track.flac","First"},{source,"next.flac","Second"},{source,"third.flac","Third"}},0);while(!player.snapshot().playing&&SDL_GetTicks64()-began<1500)SDL_Delay(5);auto state=player.snapshot();assert(state.playing&&state.details.format=="FLAC"&&state.details.sampleRate==44100&&state.duration>3.9);assert(SDL_GetTicks64()-began<1000);began=SDL_GetTicks64();player.next();while(!player.snapshot().playing&&SDL_GetTicks64()-began<1000)SDL_Delay(5);assert(player.snapshot().playing&&player.snapshot().index==1);assert(SDL_GetTicks64()-began<1000);began=SDL_GetTicks64();while(player.snapshot().index!=2&&SDL_GetTicks64()-began<6000)SDL_Delay(5);assert(player.snapshot().index==2);player.stop();}
 shutdownSMB();SDL_Quit();std::cout<<"Simulated SMB latency: 16 KiB first read, idle-session reuse, exclusive concurrent leases, FLAC first PCM and Next below 1s passed\n";
}
