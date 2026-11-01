#include "ui.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
using namespace nxh;
int main(){
 SDL_setenv("SDL_VIDEODRIVER","dummy",1);assert(!SDL_Init(SDL_INIT_VIDEO));assert(!TTF_Init());
 auto *window=SDL_CreateWindow("Artwork fit",0,0,1920,1080,0);auto *renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_SOFTWARE);assert(renderer);
 std::ifstream f("assets/font.ttf",std::ios::binary);std::vector<char> font((std::istreambuf_iterator<char>(f)),{});
 {UI ui(renderer,font.data(),font.size());for(bool docked:{false,true}){ui.setDocked(docked);for(bool card:{false,true})for(auto dimensions:{std::pair<int,int>{512,512},{1024,512},{512,1024}}){
  auto *surface=SDL_CreateRGBSurfaceWithFormat(0,dimensions.first,dimensions.second,32,SDL_PIXELFORMAT_RGBA32);assert(surface);auto *texture=ui.coverPrepared(surface,{});SDL_FreeSurface(surface);assert(texture);
  int left=card?84:64,top=142,height=537+ui.layout().extraHeight()-top,available=ui.layout().x(560-24)-ui.layout().x(left);
  ui.recordScreen(true);ui.begin();ui.imageWithin(texture,left,top,560-left-24,height);auto frame=ui.screenFrame();bool seen=false;
  for(const auto &op:frame["ops"])if(op[0]==2){int x=op[1],y=op[2],w=op[3],h=op[4];assert(x==ui.layout().x(left));assert(w<=available&&h<=height);assert(std::abs((y-top)*2+h-height)<=1);assert(std::abs(double(w)/h-double(dimensions.first)/dimensions.second)<.01);assert(w>=available-1||h>=height-1);seen=true;}assert(seen);ui.releaseCover(texture);
 }}}
 SDL_DestroyRenderer(renderer);SDL_DestroyWindow(window);TTF_Quit();SDL_Quit();std::cout<<"Square, wide and tall artwork: aspect ratio, column gap, vertical centering and card alignment at 720p/1080p passed\n";
}
