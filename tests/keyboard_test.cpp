#include "keyboard.hpp"
#include "platform.hpp"
#include "ui.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
namespace {u64 held=0,previous=0;}
void padUpdate(PadState *p){p->held=held;p->down=held&~previous;previous=held;}
int main(){
    SDL_setenv("SDL_VIDEODRIVER","dummy",1);assert(SDL_Init(SDL_INIT_VIDEO)==0);assert(TTF_Init()==0);
    auto *w=SDL_CreateWindow("keyboard test",0,0,1920,1080,0);
    auto *r=SDL_CreateRenderer(w,-1,SDL_RENDERER_SOFTWARE);assert(r);
    std::ifstream f("assets/font.ttf",std::ios::binary);std::vector<char> font((std::istreambuf_iterator<char>(f)),{});assert(!font.empty());
    {
        nxh::UI ui(r,font.data(),font.size());ui.setDocked(true);
        auto run=[&](std::string &text,const std::vector<u64> &presses,bool password=false){
            held=previous=0;size_t frame=0;
            nxh::bindKeyboard(&ui,r,[&]{assert(frame<presses.size()*2+2);held=frame%2?0:presses.at(frame/2);++frame;});
            return nxh::prosperoKeyboard("Keyboard shortcuts",text,password);
        };
        // Square removes the complete UTF-8 character, Triangle inserts a space,
        // R1 moves to uppercase and Cross types the selected Q.
        std::string text="a\xe3\x81\x82";
        assert(run(text,{HidNpadButton_Y,HidNpadButton_X,HidNpadButton_R,HidNpadButton_A,HidNpadButton_Plus}));assert(text=="a Q");
        text="";assert(run(text,{HidNpadButton_L,HidNpadButton_A,HidNpadButton_Plus}));assert(text=="-");
        text="keep";assert(!run(text,{HidNpadButton_X,HidNpadButton_B},true));assert(text=="keep");
        text="";assert(run(text,{HidNpadButton_R,HidNpadButton_L,HidNpadButton_A,HidNpadButton_Plus}));assert(text=="q");
        nxh::bindKeyboard(nullptr,nullptr,{});
    }
    SDL_DestroyRenderer(r);SDL_DestroyWindow(w);TTF_Quit();SDL_Quit();
    std::cout<<"Keyboard shortcuts, page cycling, UTF-8 deletion, acceptance and cancel passed.\n";
}
