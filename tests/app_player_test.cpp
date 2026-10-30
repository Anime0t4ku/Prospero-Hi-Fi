#define main prospero_preview_entry
#include "../source/main.cpp"
#undef main
#include <cassert>
#include <iostream>
#include <filesystem>
namespace fs=std::filesystem;
static bool hasText(UI &ui,const std::string &needle){
    auto frame=ui.screenFrame();
    for(auto &op:frame["ops"])if(op.is_array()&&op.size()>3&&op[0]==1&&op[3].is_string()&&op[3].get<std::string>().find(needle)!=std::string::npos)return true;
    return false;
}
static int textCount(UI &ui,const std::string &text){int count=0;auto frame=ui.screenFrame();for(auto &op:frame["ops"])if(op.is_array()&&op.size()>3&&op[0]==1&&op[3]==text)count++;return count;}
static void durationEdge(UI &ui,const std::string &value,int right,int size){
    int width=0,height=0;TTF_SizeUTF8(ui.font(size),value.c_str(),&width,&height);
    // This integration fixture uses a 1920x1080 output and 1536x864 logical UI.
    int logicalWidth=int(std::ceil(width/1.25));bool seen=false;
    auto frame=ui.screenFrame();for(auto &op:frame["ops"])if(op.is_array()&&op.size()>3&&op[0]==1&&op[3]==value){
        assert(op[1].get<int>()+logicalWidth==ui.layout().x(right));seen=true;
    }assert(seen);
}
static void leftTitle(UI &ui,int x){bool seen=false;auto frame=ui.screenFrame();for(auto &op:frame["ops"])if(op.is_array()&&op.size()>3&&op[0]==1&&op[3]=="Nothing playing"){assert(op[1].get<int>()==ui.layout().x(x));seen=true;}assert(seen);}
int main(){
    SDL_setenv("SDL_VIDEODRIVER","dummy",1);SDL_setenv("SDL_AUDIODRIVER","dummy",1);
    assert(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO)==0);assert(TTF_Init()==0);
    auto *window=SDL_CreateWindow("Player integration",0,0,1920,1080,0);auto *renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_SOFTWARE);assert(renderer);
    std::ifstream f("assets/font.ttf",std::ios::binary);std::vector<char> font((std::istreambuf_iterator<char>(f)),{});assert(!font.empty());
    char directoryTemplate[]="/tmp/prospero-player-app-XXXXXX";
    const char *directory=mkdtemp(directoryTemplate);assert(directory);
    const std::string root=directory;
    // A small real PCM WAV exercises file selection and the asynchronous browser/player path.
    {std::ofstream wav(root+"/test.wav",std::ios::binary);auto le=[&](unsigned value,int count){for(int i=0;i<count;i++)wav.put(char(value>>(8*i)));};
     wav.write("RIFF",4);le(36+176400,4);wav.write("WAVEfmt ",8);le(16,4);le(1,2);le(2,2);le(44100,4);le(176400,4);le(4,2);le(16,2);wav.write("data",4);le(176400,4);std::string silence(176400,'\0');wav.write(silence.data(),silence.size());}
    for(int i=1;i<=40;i++)fs::copy_file(root+"/test.wav",root+"/Track "+std::to_string(i)+" - browser redraw.wav");
    for(bool automatic:{false,true}){
        UI ui(renderer,font.data(),font.size());ui.setDocked(true);Settings settings;settings.musicRoot=root;settings.remoteEnabled=false;settings.autoFullscreen=automatic;
        ui.recordScreen(true);ui.begin();ui.badge(100,100,200,38,"44.1 kHz",16);
        auto metadata=ui.screenFrame();bool centered=false;
        for(auto &op:metadata["ops"])if(op.is_array()&&op.size()>3&&op[0]==1&&op[3]=="44.1 kHz"){
            assert(op[1].get<int>()>ui.layout().x(100)&&op[1].get<int>()<ui.layout().x(200));assert(op[2].get<int>()>100);centered=true;
        }
        assert(centered);
        for(int size:{14,18})for(auto value:{"--:--","1:23","~12:34","123:45:56"}){
            ui.begin();ui.rightText(1228,100,value,size,MUTED);durationEdge(ui,value,1228,size);
        }
        Player player(settings);assert(player.valid());App app(ui,renderer,player,settings,"",root+"/config.json");PadState pad{};
        auto press=[&](u64 button){app.input(pad,button,button,true);app.input(pad,0,0,true);};
        auto draw=[&]{ui.recordScreen(true);app.draw();};
        draw();assert(hasText(ui,"Sources"));durationEdge(ui,"--:--",1228,14);leftTitle(ui,900);
        // Settings: categories require Cross, Circle returns to the same category.
        press(HidNpadButton_L);draw();
        assert(hasText(ui,"CATEGORIES")&&hasText(ui,"Accent colour")&&!hasText(ui,"Appearance /"));
        if(!automatic)if(const char *path=SDL_getenv("PROSPERO_TEST_SETTINGS_IMAGE")){
            auto *surface=SDL_CreateRGBSurfaceWithFormat(0,1920,1080,32,SDL_PIXELFORMAT_RGBA32);
            assert(SDL_RenderReadPixels(renderer,nullptr,surface->format->format,surface->pixels,surface->pitch)==0);
            assert(IMG_SavePNG(surface,path)==0);SDL_FreeSurface(surface);
        }
        press(HidNpadButton_Right);draw();assert(!hasText(ui,"OLED")); // No choice dialog while on categories.
        press(HidNpadButton_A);draw();assert(hasText(ui,"Categories"));
        press(HidNpadButton_A);draw();assert(hasText(ui,"Dark")&&hasText(ui,"OLED"));
        press(HidNpadButton_B);draw();assert(!hasText(ui,"OLED")); // Choice dismissal stays in entries.
        press(HidNpadButton_Down); // Accent colour.
        press(HidNpadButton_B);draw();assert(hasText(ui,"Select"));
        press(HidNpadButton_Down);draw();assert(hasText(ui,"Show artwork")&&!hasText(ui,"Accent colour"));
        press(HidNpadButton_A);draw();const int artworkOn=textCount(ui,"On");
        press(HidNpadButton_A);draw();assert(textCount(ui,"On")==artworkOn-1);
        press(HidNpadButton_A);draw();assert(textCount(ui,"On")==artworkOn);
        press(HidNpadButton_Minus);draw();assert(hasText(ui,"Browse"));
        press(HidNpadButton_B);draw();assert(hasText(ui,"Categories")); // Mini player preserves entries.
        press(HidNpadButton_B);press(HidNpadButton_Up);press(HidNpadButton_A);
        press(HidNpadButton_A);draw();assert(hasText(ui,"Save colour")); // Appearance remembers Accent colour.
        press(HidNpadButton_B);press(HidNpadButton_B);press(HidNpadButton_R);draw();
        assert(hasText(ui,"MUSIC SOURCES"));
        press(HidNpadButton_StickR);draw();assert(!hasText(ui,"Sources")&&hasText(ui,"Play/Pause")&&!hasText(ui,"Seek 10s"));
        durationEdge(ui,"--:--",1216,18);leftTitle(ui,560);
        auto fullFrame=ui.screenFrame();for(auto &op:fullFrame["ops"])if(op.is_array()&&op.size()>4&&op[0]==0)assert(!(op[1]==ui.layout().x(544)&&op[2]==138));
        assert(textCount(ui,"Play/Pause")==2); // Focused Cross action plus Options shortcut.
        assert(!hasText(ui,"Cross")&&!hasText(ui,"Circle")&&!hasText(ui,"Select a track"));
        assert(!hasText(ui,"Previous")); // No labels underneath fullscreen buttons.
        press(HidNpadButton_L);press(HidNpadButton_R);draw();assert(!hasText(ui,"Sources"));
        press(HidNpadButton_Minus);draw();assert(!hasText(ui,"Sources"));
        press(HidNpadButton_Up);draw();assert(hasText(ui,"Volume"));
        press(HidNpadButton_Up);draw();assert(hasText(ui,"Seek 10s"));
        press(HidNpadButton_Down);press(HidNpadButton_Down);draw();assert(hasText(ui,"Next"));
        press(HidNpadButton_Right);press(HidNpadButton_Right);press(HidNpadButton_Right);press(HidNpadButton_A);
        draw();assert(hasText(ui,"Equalizer preset")&&!hasText(ui,"Sources"));
        press(HidNpadButton_B);draw();assert(!hasText(ui,"Equalizer preset")&&!hasText(ui,"Sources"));
        Source local{false,{},root};player.play({{local,"test.wav","One"},{local,"test.wav","Two"},{local,"test.wav","Three"}},1);
        press(HidNpadButton_L);assert(player.snapshot().index==0);
        press(HidNpadButton_R);assert(player.snapshot().index==1);
        press(HidNpadButton_ZL);assert(player.snapshot().index==0);
        press(HidNpadButton_ZR);assert(player.snapshot().index==1);
        press(HidNpadButton_L|HidNpadButton_ZL);assert(player.snapshot().index==0);
        press(HidNpadButton_R|HidNpadButton_ZR);assert(player.snapshot().index==1);
        player.stop();
        press(HidNpadButton_B);draw();assert(hasText(ui,"Sources"));
        press(HidNpadButton_R);draw();assert(player.snapshot().index==1);assert(hasText(ui,"SELECT A MUSIC SOURCE"));
        press(HidNpadButton_L);draw();assert(player.snapshot().index==1&&hasText(ui,"MUSIC SOURCES"));
        press(HidNpadButton_Minus);press(HidNpadButton_Up);draw();assert(hasText(ui,"Seek 10s"));
        press(HidNpadButton_B);draw();assert(hasText(ui,"Open"));
        press(HidNpadButton_A);
        for(int i=0;i<80;i++){app.update();SDL_Delay(5);}
        press(HidNpadButton_A);draw();assert(hasText(ui,"Sources")!=automatic);
        if(automatic)press(HidNpadButton_B);
        player.next();app.update();draw();assert(hasText(ui,"Sources"));
    }
    {
        UI ui(renderer,font.data(),font.size());ui.setDocked(true);
        ui.recordScreen(true);
        Settings settings;settings.musicRoot=root;settings.remoteEnabled=false;
        Player player(settings);App app(ui,renderer,player,settings,"",root+"/config.json");PadState pad{};
        auto press=[&](u64 button){app.input(pad,button,button,true);app.input(pad,0,0,true);};
        auto draw=[&]{ui.recordScreen(true);app.draw();};
        press(HidNpadButton_A);
        for(int i=0;i<80;i++){app.update();SDL_Delay(5);}
        draw();assert(hasText(ui,"Track 1 - browser redraw.wav"));
        press(HidNpadButton_A); // Keep playback active during focus switches and scrolling.
        SDL_Rect browser{28*1920/1280,155*1080/864,828*1920/1280,650*1080/864};
        std::vector<Uint32> normal(browser.w*browser.h),fresh(normal.size());
        for(int frame=0;frame<90;frame++){
            if(frame%3==0){press(HidNpadButton_Minus);draw();press(HidNpadButton_B);}
            press(frame<60?HidNpadButton_Down:HidNpadButton_Up);draw();
            assert(SDL_RenderReadPixels(renderer,&browser,SDL_PIXELFORMAT_ARGB8888,normal.data(),browser.w*4)==0);
            // Poison the old frame and leave a stale clip before drawing the same view.
            SDL_SetRenderDrawColor(renderer,255,0,255,255);SDL_RenderClear(renderer);
            SDL_Rect stale{100,100,20,20};SDL_RenderSetClipRect(renderer,&stale);draw();
            assert(!SDL_RenderIsClipEnabled(renderer));
            assert(SDL_RenderReadPixels(renderer,&browser,SDL_PIXELFORMAT_ARGB8888,fresh.data(),browser.w*4)==0);
            assert(normal==fresh);
        }
        player.stop();
    }
    fs::remove_all(root);SDL_DestroyRenderer(renderer);SDL_DestroyWindow(window);TTF_Quit();SDL_Quit();
    std::cout<<"App integration: Settings category/entry controls and remembered focus; active-playback browser redraw/clip recovery (90 frames); fullscreen shortcuts, Auto Fullscreen and duration alignment passed.\n";
}
