#include "audio_analysis.hpp"
#include <cassert>
#include <iostream>
#include <pthread.h>
using namespace nxh;
static AudioVisualFrame tone(double hz,bool anti=false){
    AudioAnalysis a;AudioVisualFrame last;std::vector<float> pcm(2048*2);
    for(unsigned block=0;block<14;block++){
        for(unsigned i=0;i<2048;i++){float v=.8f*std::sin(2*3.141592653589793*hz*(block*2048+i)/AudioAnalysis::Rate);pcm[i*2]=v;pcm[i*2+1]=anti?-v:v;}
        auto f=a.push(pcm.data(),2048);if(!f.empty())last=f.back();
    }return last;
}
static void *smallStack(void *){
    AudioAnalysis a;std::vector<float> pcm(8192*2,.25f);
    auto frames=a.push(pcm.data(),8192);assert(!frames.empty());
    a.reset(44100);frames=a.push(pcm.data(),8192);assert(frames.front().frame==44100);
    return nullptr;
}
int main(){
    pthread_attr_t attr;pthread_t thread;assert(pthread_attr_init(&attr)==0);
    assert(pthread_attr_setstacksize(&attr,64*1024)==0);
    assert(pthread_create(&thread,&attr,smallStack,nullptr)==0);
    assert(pthread_attr_destroy(&attr)==0);assert(pthread_join(thread,nullptr)==0);
    assert(sizeof(AudioAnalysis)<512);
    for(double hz:{60.,1000.,8000.,16000.}){
        auto frame=tone(hz);int peak=int(std::max_element(frame.spectrum.begin(),frame.spectrum.end())-frame.spectrum.begin());
        double center=20*std::pow(1000.,(peak+.5)/32.);assert(center>hz*.7&&center<hz*1.4);assert(frame.spectrum[peak]>.75f);
    }
    auto stereo=tone(1000),anti=tone(1000,true);
    for(unsigned i=0;i<32;i++)assert(std::abs(stereo.spectrum[i]-anti.spectrum[i])<1e-5f);
    for(unsigned i=0;i<256;i++)assert(std::abs(anti.left[i]+anti.right[i])<1e-5f);
    AudioAnalysis a;std::vector<float> pcm(8192*2);pcm[1000*2]=pcm[1000*2+1]=1;
    auto f=a.push(pcm.data(),8192);assert(f.size()==13);
    assert(std::any_of(f.begin(),f.end(),[](auto &v){return *std::max_element(v.spectrum.begin(),v.spectrum.end())>.1f;})); // Transient outside old first-256 capture.
    for(size_t i=1;i<f.size();i++)assert(f[i].frame-f[i-1].frame==512);
    a.reset(44100);std::fill(pcm.begin(),pcm.end(),0);f=a.push(pcm.data(),8192);assert(f.front().frame==44100);
    for(auto &frame:f)for(auto value:frame.spectrum)assert(value==0);
    a.reset();std::fill(pcm.begin(),pcm.end(),.3f);f=a.push(pcm.data(),8192);for(auto value:f.back().spectrum)assert(value<.01f); // DC removed.
    AudioVisualQueue queue;AudioVisualFrame first,next;first.frame=0;next.frame=512;next.spectrum.fill(1);next.left.fill(.8f);
    queue.publish({first,next},22050);auto value=queue.sample(22050-1024-256);assert(std::abs(value.spectrum[0]-.5f)<1e-6f);assert(value.left[0]==0); // Audible cursor, not latest decoded data.
    value=queue.sample(22050-1024-512);assert(value.spectrum[0]==1&&value.left[0]==.8f);
    queue.reset(44100);value=queue.sample(999999);assert(value.frame==44100&&value.spectrum[0]==0);
    std::cout<<"64 KiB thread-stack regression; continuous STFT: bass/mid/treble tones, anti-phase stereo, missed-region transient, hop cadence, seek reset, silence/DC, and audible-queue alignment passed.\n";
}
