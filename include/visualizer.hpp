#pragma once
#include "ui.hpp"
#include "player.hpp"
#include "player_navigation.hpp"
#include <deque>
namespace nxh {
inline constexpr const char *visualizerNames[]={"Stock spectrum","Spectrum mountains","Mirrored spectrum","Waveform ribbon","Radial spectrum","Spectrogram","Particle spectrum","Oscilloscope","Stereo scope","Spectrum waterfall","Dot matrix","Orbit","Bass ripples","Frequency strings","Pixel skyline","Twin waves","Starfield","Pulse lattice","Flame spectrum"};
// Bounded, frame-rate independent state; no texture or allocation per particle.
class Visualizer {
    std::array<float,32> bands{},peaks{};
    std::deque<std::array<float,32>> history;
    Uint64 last=0,historyAt=0;
    std::string track;
    float phase=0;
    int previousStyle=-1;
    static Color blend(Color a,Color b,float t){t=std::clamp(t,0.f,1.f);return {Uint8(a.r+(b.r-a.r)*t),Uint8(a.g+(b.g-a.g)*t),Uint8(a.b+(b.b-a.b)*t)};}
  public:
    void draw(UI &ui,const Playback &p,PlayerRect area,const Settings &s){
        const auto now=SDL_GetTicks64();int style=std::clamp(s.visualizerStyle,0,18);
        if(track!=p.track.path||style!=previousStyle){bands={};peaks={};history.clear();track=p.track.path;previousStyle=style;last=0;historyAt=0;phase=0;}
        float dt=last?std::min(.1f,float(now-last)/1000):1.f/60;last=now;
        bool active=p.playing&&!p.paused&&!p.busy&&!p.ended;
        if(active)phase+=dt;
        float tau=.015f+s.visualizerSmoothing*.004f,alpha=1-std::exp(-dt/tau);
        float energy=0,bass=0;
        for(int i=0;i<32;i++){
            float target=active?std::clamp(p.spectrum[i]*s.visualizerSensitivity/100.f,0.f,1.f):0;
            bands[i]+=(target-bands[i])*alpha;energy+=bands[i]/32;
            peaks[i]=std::max(bands[i],peaks[i]-dt*.4f);if(i<5)bass+=bands[i]/5;
        }
        if(active&&now-historyAt>=40){history.push_front(bands);if(history.size()>96)history.pop_back();historyAt=now;}
        if(!active&&!p.paused&&energy<.005f)history.clear();
        int x=area.x,y=area.y,w=area.w,h=area.h;
        if(w<4||h<4)return;
        UI::Clip clip(ui,x,y,w,h); // Transparent: preserve the existing player backdrop.
        auto ink=[&](float strength){return blend(BG,ACCENT,strength);};
        auto line=[&](float a,float b,float c,float d,Color color){ui.visualizerLine(int(a),int(b),int(c),int(d),color);};
        auto sample=[&](float f){float v=std::clamp(f,0.f,1.f)*31;int i=int(v);return bands[i]+(bands[std::min(31,i+1)]-bands[i])*(v-i);};
        auto wave=[&](int i,bool right){return active?std::clamp((right?p.waveRight[i]:p.waveLeft[i])*s.visualizerSensitivity/100.f,-1.f,1.f):0.f;};
        auto ring=[&](float cx,float cy,float radius,float strength,int lobes=0){
            for(int i=0;i<128;i++){float a=i*6.2831853f/128,b=(i+1)*6.2831853f/128;
                float ra=radius+(lobes?sample(i/128.f)*h*.12f:0),rb=radius+(lobes?sample((i+1)/128.f)*h*.12f:0);
                line(cx+std::cos(a)*ra,cy+std::sin(a)*ra,cx+std::cos(b)*rb,cy+std::sin(b)*rb,ink(strength));}
        };
        if(style==0||style==2||style==14||style==18){
            for(int i=0;i<32;i++){
                int bw=std::max(2,w/32-3),bh=std::max(2,int(bands[i]*h));int bx=x+i*w/32;
                if(style==2){bh/=2;ui.rect(bx,y+h/2-bh,bw,std::max(2,bh*2),ACCENT);}
                else if(style==14){for(int by=0;by<bh;by+=8)ui.rect(bx,y+h-by-6,bw,5,ink(.35f+.65f*by/h));ui.rect(bx,y+h-int(peaks[i]*h),bw,2,TEXT);}
                else if(style==18){for(int by=0;by<bh;by+=4){float t=float(by)/std::max(1,bh);Color c=blend({220,40,15},{255,226,85},t);ui.rect(bx,y+h-by-4,bw,4,c);}}
                else ui.rect(bx,y+h-bh,bw,bh,ACCENT);
            }return;
        }
        if(style==1||style==9){
            int layers=style==1?5:std::min(24,int(history.size()));
            for(int layer=layers-1;layer>=0;layer--){
                float depth=float(layer)/std::max(1,layers),base=y+h-(style==9?depth*h*.65f:layer*h*.09f);
                float inset=style==9?depth*w*.22f:0;Color color=ink(1-depth*.8f);
                for(int i=0;i<127;i++){
                    auto level=[&](int n){if(style==1)return sample(n/127.f);auto &v=history[std::min(int(history.size())-1,layer*3)];float f=n*31.f/127;int k=int(f);return v[k]+(v[std::min(31,k+1)]-v[k])*(f-k);};
                    line(x+inset+i*(w-2*inset)/127,base-level(i)*h*(style==1?.52f:.3f),x+inset+(i+1)*(w-2*inset)/127,base-level(i+1)*h*(style==1?.52f:.3f),color);
                }
            }return;
        }
        if(style==3||style==7||style==15){
            int layers=style==3?6:style==15?2:1;
            for(int l=layers-1;l>=0;l--)for(int i=0;i<255;i++){
                float center=style==15?y+h*(l?.75f:.25f):y+h*.5f;
                float amp=h*(style==15?.22f:.44f)*(style==3?1-l*.095f:1);
                float offset=style==3?l*h*.012f:0;
                line(x+i*w/255.f,center+wave(i,style==15&&l==1)*amp+offset,x+(i+1)*w/255.f,center+wave(i+1,style==15&&l==1)*amp+offset,ink(1-l*.13f));
            }return;
        }
        if(style==8){for(int i=0;i<255;i++)line(x+w*.5f+wave(i,false)*w*.45f,y+h*.5f+wave(i,true)*h*.45f,x+w*.5f+wave(i+1,false)*w*.45f,y+h*.5f+wave(i+1,true)*h*.45f,ink(.4f+i/425.f));return;}
        if(style==4||style==11||style==12){
            float radius=std::min(w,h)*.28f,cx=x+w*.5f,cy=y+h*.5f;
            if(style==4){ring(cx,cy,radius,.35f);for(int i=0;i<64;i++){float a=i*6.2831853f/64;float r=radius+sample(i/63.f)*std::min(w,h)*.2f;line(cx+cos(a)*radius,cy+sin(a)*radius,cx+cos(a)*r,cy+sin(a)*r,ACCENT);}}
            if(style==11)for(int l=0;l<5;l++){float angle=phase*.3f+l;ring(cx+cos(angle)*w*.05f*energy,cy+sin(angle)*h*.05f*energy,radius*(.35f+l*.19f),.25f+l*.15f,1);}
            if(style==12)for(int l=0;l<7;l++){float f=fmod(phase*.24f+l/7.f,1.f);ring(cx,cy,f*std::min(w,h)*.48f,(1-f)*(.15f+bass*.85f));}
            return;
        }
        if(style==5){
            for(int col=0;col<int(history.size());col++)for(int b=0;b<32;b++){
                float v=history[col][b];Color c=v<.5f?blend({10,12,40},{30,80,220},v*2):blend({30,80,220},{255,214,64},(v-.5f)*2);
                int cw=std::max(1,(w+95)/96),ch=std::max(1,(h+31)/32);ui.rect(x+w-(col+1)*cw,y+h-(b+1)*ch,cw,ch,c);
            }return;
        }
        if(style==10){for(int i=0;i<32;i++)for(int j=0;j<12;j++){float v=std::clamp(bands[i]*12-j,0.f,1.f);int size=std::max(2,int(std::min(w/32.f,h/12.f)*.55f));ui.rect(x+i*w/32+(w/32-size)/2,y+h-(j+1)*h/12,size,size,ink(.08f+.92f*v));}return;}
        if(style==13){for(int l=0;l<12;l++)for(int i=0;i<95;i++){
            auto point=[&](int n){return y+(l+.5f)*h/12+sin(n*.16f+phase*3+l)*sample(l/11.f)*h*.07f;};
            line(x+i*w/95.f,point(i),x+(i+1)*w/95.f,point(i+1),ink(.3f+.7f*sample(l/11.f)));}return;}
        if(style==17){
            auto pt=[&](int i,int j){return std::array<float,2>{x+i*w/24.f,y+(j+.5f)*h/9+sin(i*.4f+j+phase*2)*sample(i/24.f)*h*.07f};};
            for(int j=0;j<9;j++)for(int i=0;i<=24;i++){auto a=pt(i,j);if(i<24){auto b=pt(i+1,j);line(a[0],a[1],b[0],b[1],ink(.25f+.65f*sample(i/24.f)));}if(j<8){auto b=pt(i,j+1);line(a[0],a[1],b[0],b[1],ink(.2f));}}return;
        }
        // Deterministic particle seeds give stable motion without per-frame randomness.
        for(int i=0;i<112;i++){
            float seed=fmod(i*.618034f,1.f),level=bands[i%32];
            if(style==16){float z=1-fmod(seed+phase*(.08f+energy*.2f),1.f),a=i*2.399963f;float d=.04f/std::max(.04f,z);float cx=x+w*.5f+cos(a)*w*d,cy=y+h*.5f+sin(a)*h*d;int size=std::clamp(int(d*4),1,7);ui.rect(int(cx),int(cy),size,size,ink(std::min(1.f,energy+.2f+(1-z)*.6f)));}
            else{float bx=x+fmod(seed*w+sin(phase+i)*level*w*.035f,float(w)),by=y+h-fmod(i*19.3f+phase*(18+level*100),float(h));int size=std::max(1,int(2+level*5));ui.rect(int(bx),int(by),size,size,ink(level));}
        }
    }
};
}
