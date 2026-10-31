#include "visualizer.hpp"
#include <cassert>
#include <fstream>
#include <filesystem>
#include <iostream>
using namespace nxh;
int main(){
 SDL_setenv("SDL_VIDEODRIVER","dummy",1);assert(SDL_Init(SDL_INIT_VIDEO)==0);assert(TTF_Init()==0);
 auto *window=SDL_CreateWindow("Visualizer test",0,0,1280,720,0);auto *renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_SOFTWARE);assert(renderer);
 std::ifstream f("assets/font.ttf",std::ios::binary);std::vector<char> font((std::istreambuf_iterator<char>(f)),{});
 {
 UI ui(renderer,font.data(),font.size());ui.setDocked(false);Playback p;p.playing=true;p.track.path="fixture.wav";
 for(int i=0;i<32;i++)p.spectrum[i]=.2f+.7f*std::abs(std::sin(i*.31f));
 for(int i=0;i<256;i++){p.waveLeft[i]=sin(i*.13f)*.8f;p.waveRight[i]=sin(i*.13f+.9f)*.7f;}
 std::vector<Uint32> pixels(1280*720);
 for(auto theme:{"dark","light","oled"})for(int style=0;style<19;style++){
    Settings s;s.theme=theme;s.visualizerStyle=style;setAppearance(theme,s.accent);Visualizer visualizer;
    ui.begin();ui.recordScreen(true);auto frame=ui.screenFrame();
    std::vector<Uint32> before(1280*720);assert(SDL_RenderReadPixels(renderer,nullptr,SDL_PIXELFORMAT_ARGB8888,before.data(),1280*4)==0);
    // Advanced draw calls are local only; caller disables recording.
    ui.recordScreen(false);
    for(int i=0;i<90;i++){visualizer.draw(ui,p,{64,180,1152,360},s);SDL_Delay(5);}
    assert(!SDL_RenderIsClipEnabled(renderer));assert(ui.screenFrame()==frame);
    assert(SDL_RenderReadPixels(renderer,nullptr,SDL_PIXELFORMAT_ARGB8888,pixels.data(),1280*4)==0);
    auto bg=pixels[0];int changed=0;
    for(int y=0;y<720;y++)for(int x=0;x<1280;x++){
       if(x<64||x>=1216||y<180||y>=540)assert(pixels[y*1280+x]==before[y*1280+x]);
       else if(pixels[y*1280+x]!=bg)changed++;
    }
    assert(changed>30);
    if(style==7)assert(pixels[190*1280+1100]==before[190*1280+1100]); // Empty interior keeps the original backdrop.
    if(std::string(theme)=="dark")if(auto dir=std::getenv("PROSPERO_VISUALIZER_PREVIEW")){
       std::filesystem::create_directories(dir);auto *surface=SDL_CreateRGBSurfaceWithFormat(0,1280,720,32,SDL_PIXELFORMAT_ARGB8888);
       assert(SDL_RenderReadPixels(renderer,nullptr,surface->format->format,surface->pixels,surface->pitch)==0);
       assert(IMG_SavePNG(surface,(std::string(dir)+"/style-"+std::to_string(style)+".png").c_str())==0);SDL_FreeSurface(surface);
    }
    p.paused=true;visualizer.draw(ui,p,{64,180,1152,360},s);p.paused=false;
    p.playing=false;visualizer.draw(ui,p,{64,180,1152,360},s);p.playing=true;
    visualizer.draw(ui,p,{0,0,1280,720},s);assert(!SDL_RenderIsClipEnabled(renderer));
 }
 }
 SDL_DestroyRenderer(renderer);SDL_DestroyWindow(window);TTF_Quit();SDL_Quit();
 std::cout<<"All 19 styles render in dark/light/OLED themes, respect bounds, handle pause/stop and expanded sizes, and do not change remote frames.\n";
}
