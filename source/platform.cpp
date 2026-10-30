#include "platform.hpp"
#include <cstdio>
#include <cstring>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#ifndef __PROSPERO__
#include <ifaddrs.h>
#include <net/if.h>
#endif
#ifdef __PROSPERO__
extern "C" int ProsperoSDLFollowOutput(void);
extern "C" int sceNetInit(void);
extern "C" int sceNetCtlInit(void);
extern "C" int sceNetCtlGetInfo(int,void *);
#endif
namespace {
SDL_GameController *controller=nullptr;
u64 previous=0;
bool storageReady=false;
}
void padUpdate(PadState *p) {
    SDL_PumpEvents();
    if(controller && !SDL_GameControllerGetAttached(controller)){SDL_GameControllerClose(controller);controller=nullptr;}
    if(!controller)for(int i=0;i<SDL_NumJoysticks();++i)if(SDL_IsGameController(i)){controller=SDL_GameControllerOpen(i);if(controller)break;}
    u64 bits=0;
    auto button=[&](SDL_GameControllerButton b,u64 mask){if(controller&&SDL_GameControllerGetButton(controller,b))bits|=mask;};
    button(SDL_CONTROLLER_BUTTON_A,HidNpadButton_A);button(SDL_CONTROLLER_BUTTON_B,HidNpadButton_B);
    button(SDL_CONTROLLER_BUTTON_Y,HidNpadButton_X);button(SDL_CONTROLLER_BUTTON_X,HidNpadButton_Y);
    button(SDL_CONTROLLER_BUTTON_START,HidNpadButton_Plus);button(SDL_CONTROLLER_BUTTON_TOUCHPAD,HidNpadButton_Minus);
    button(SDL_CONTROLLER_BUTTON_BACK,HidNpadButton_Minus);button(SDL_CONTROLLER_BUTTON_LEFTSHOULDER,HidNpadButton_L);
    button(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,HidNpadButton_R);button(SDL_CONTROLLER_BUTTON_LEFTSTICK,HidNpadButton_StickL);button(SDL_CONTROLLER_BUTTON_RIGHTSTICK,HidNpadButton_StickR);
    button(SDL_CONTROLLER_BUTTON_DPAD_UP,HidNpadButton_Up);button(SDL_CONTROLLER_BUTTON_DPAD_DOWN,HidNpadButton_Down);
    button(SDL_CONTROLLER_BUTTON_DPAD_LEFT,HidNpadButton_Left);button(SDL_CONTROLLER_BUTTON_DPAD_RIGHT,HidNpadButton_Right);
    if(controller){
        if(SDL_GameControllerGetAxis(controller,SDL_CONTROLLER_AXIS_TRIGGERLEFT)>10000)bits|=HidNpadButton_ZL;
        if(SDL_GameControllerGetAxis(controller,SDL_CONTROLLER_AXIS_TRIGGERRIGHT)>10000)bits|=HidNpadButton_ZR;
        int x=SDL_GameControllerGetAxis(controller,SDL_CONTROLLER_AXIS_LEFTX),y=SDL_GameControllerGetAxis(controller,SDL_CONTROLLER_AXIS_LEFTY);
        if(x<-12000)bits|=HidNpadButton_Left;if(x>12000)bits|=HidNpadButton_Right;
        if(y<-12000)bits|=HidNpadButton_Up;if(y>12000)bits|=HidNpadButton_Down;
    }
    const Uint8 *k=SDL_GetKeyboardState(nullptr);
    for(auto b:{std::pair<SDL_Scancode,u64>{SDL_SCANCODE_RETURN,HidNpadButton_A},{SDL_SCANCODE_ESCAPE,HidNpadButton_B},
        {SDL_SCANCODE_UP,HidNpadButton_Up},{SDL_SCANCODE_DOWN,HidNpadButton_Down},{SDL_SCANCODE_LEFT,HidNpadButton_Left},
        {SDL_SCANCODE_RIGHT,HidNpadButton_Right},{SDL_SCANCODE_SPACE,HidNpadButton_Plus},{SDL_SCANCODE_TAB,HidNpadButton_Minus},
        {SDL_SCANCODE_Q,HidNpadButton_L},{SDL_SCANCODE_E,HidNpadButton_R},{SDL_SCANCODE_F,HidNpadButton_StickR},{SDL_SCANCODE_G,HidNpadButton_StickL}})
        if(k[b.first])bits|=b.second;
    p->held=bits;p->down=bits&~previous;previous=bits;
}
namespace nxh {
bool ensureStorageAccess();
const char *assetRoot(){
#ifdef __PROSPERO__
    return access("/data/homebrew/PPSA99287/assets/font.ttf",R_OK)==0 ? "/data/homebrew/PPSA99287/assets/" : "/app0/assets/";
#else
    return "assets/";
#endif
}
void initPlatform(){
#ifdef __PROSPERO__
    sceNetInit();
    sceNetCtlInit();
#endif
    bool accessReady=ensureStorageAccess();storageReady=accessReady;
    std::printf("[storage] access %s\n",accessReady?"ready":"unavailable; local ELF loader is required");
    if(accessReady && access("/data/prospero-hifi",F_OK)!=0)
        std::rename("/data/propero-hifi","/data/prospero-hifi");
    mkdir("/data/prospero-hifi",0777);mkdir("/data/prospero-hifi/cd-cache",0777);mkdir("/data/music",0777);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY,"linear");
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS,"1");
}
std::string localNetworkAddress(){
    auto valid=[](const char *ip){
        in_addr a{};if(inet_pton(AF_INET,ip,&a)!=1)return false;
        auto host=ntohl(a.s_addr);
        return host!=0 && host!=0xffffffffu && (host>>24)!=127;
    };
#ifdef __PROSPERO__
    // SceNetCtlInfo IP address selector, also used by PS5SX2's network adapter.
    alignas(8) char info[512]{};
    if(sceNetCtlGetInfo(14,info)==0){info[sizeof(info)-1]=0;if(valid(info))return info;}
#else
    ifaddrs *interfaces=nullptr;
    if(getifaddrs(&interfaces)==0){
        std::string found;
        for(auto *i=interfaces;i;i=i->ifa_next){
            if(!i->ifa_addr||i->ifa_addr->sa_family!=AF_INET||!(i->ifa_flags&IFF_UP))continue;
            char ip[INET_ADDRSTRLEN]{};
            auto *a=reinterpret_cast<sockaddr_in*>(i->ifa_addr);
            if(inet_ntop(AF_INET,&a->sin_addr,ip,sizeof(ip))&&valid(ip)){found=ip;break;}
        }
        freeifaddrs(interfaces);return found;
    }
#endif
    return {};
}
bool storageAccessReady(){return storageReady;}
void closePlatform(){if(controller)SDL_GameControllerClose(controller);controller=nullptr;}
bool platformQuitRequested(){return false;}
bool refreshDisplay(SDL_Window *w,SDL_Renderer *r,bool force){
    static int width=0,height=0;int ww=0,hh=0;
#ifdef __PROSPERO__
    static Uint64 lastCheck=0;
    if(SDL_GetTicks64()-lastCheck>1000){lastCheck=SDL_GetTicks64();ProsperoSDLFollowOutput();}
#endif
    SDL_GetRendererOutputSize(r,&ww,&hh);
    if(ww<1||hh<1)return false;
    if(force||ww!=width||hh!=height){
        width=ww;height=hh;
        // Logical geometry stays NX-compatible; textures/fonts rasterise at the real output scale.
        SDL_RenderSetLogicalSize(r,1536,864);
        std::printf("[display] framebuffer %dx%d; logical 1536x864\n",width,height);
        return true;
    }
    return false;
}
}
