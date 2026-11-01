#include "player.hpp"
#include <cassert>
#include <iostream>
#include <chrono>
using namespace nxh;
static std::atomic<int> clears{0};
extern "C" void __real_SDL_ClearQueuedAudio(SDL_AudioDeviceID);
extern "C" void __wrap_SDL_ClearQueuedAudio(SDL_AudioDeviceID d){++clears;__real_SDL_ClearQueuedAudio(d);}
template<class F> bool until(F f,int ms=6000){auto end=SDL_GetTicks64()+ms;do{if(f())return true;SDL_Delay(5);}while(SDL_GetTicks64()<end);return false;}
int main(int argc,char **argv){assert(argc==2);SDL_setenv("SDL_AUDIODRIVER","dummy",1);assert(!SDL_Init(SDL_INIT_AUDIO|SDL_INIT_TIMER));Settings s;s.remoteEnabled=false;s.gaplessPlayback=true;Source source{false,{},argv[1]};{
 Player p(s);auto began=SDL_GetTicks64();p.play({{source,"aac.m4a","First"},{source,"alac.m4a","Second"}},0);assert(until([&]{return p.snapshot().playing;}));int initial=clears;assert(until([&]{return p.snapshot().index==1;}));assert(SDL_GetTicks64()-began>=3700);assert(clears==initial);assert(p.snapshot().playing&&!p.snapshot().busy);assert(until([&]{return p.snapshot().details.format=="ALAC";}));p.pause();assert(p.snapshot().paused);SDL_Delay(80);double position=p.snapshot().position;SDL_Delay(100);assert(std::abs(position-p.snapshot().position)<.05);p.pause();auto start=SDL_GetTicks64();p.previous();assert(until([&]{return p.snapshot().playing&&p.snapshot().index==0;}));assert(SDL_GetTicks64()-start<1500);p.stop();
 p.play({{source,"aac.m4a","First"},{source,"alac.m4a","Second"},{source,"aac.m4a","Third"}},0);assert(until([&]{return p.snapshot().playing;}));SDL_Delay(3600);assert(p.snapshot().index==0);p.next();assert(p.snapshot().index==1);assert(until([&]{return p.snapshot().playing;}));p.stop();
 s.gaplessPlayback=false;p.settings(s);p.play({{source,"aac.m4a","First"},{source,"alac.m4a","Second"}},0);assert(until([&]{return p.snapshot().ended;}));assert(p.snapshot().index==0);p.tick();assert(until([&]{return p.snapshot().playing&&p.snapshot().index==1;}));p.stop();}
 SDL_Quit();std::cout<<"File gapless handoff without SDL queue flush, pause, previous and non-gapless queue advance passed\n";}
