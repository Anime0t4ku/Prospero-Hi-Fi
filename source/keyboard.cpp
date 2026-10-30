// Controller keyboard adapted from PS5SX2's AchievementAccountPanel/PageRows,
// KeyCentre and BuildPanelKeyboard (swordpdf, GPL-3.0-or-later).
#include "keyboard.hpp"
#include "platform.hpp"
#include "ui.hpp"
#include <cstring>
namespace nxh {
namespace {
UI *ui=nullptr;SDL_Renderer *renderer=nullptr;std::function<void()> service;
constexpr const char *pages[3][4]={{"1234567890","qwertyuiop","asdfghjkl","zxcvbnm"},{"1234567890","QWERTYUIOP","ASDFGHJKL","ZXCVBNM"},{"!@#$%^&*()","-_=+[]{}\\|",";:'\",.<>/?","`~"}};
constexpr float widths[]={1.5f,1.5f,4.f,1.5f,1.5f};
int keys(int page,int row){return row==4?5:std::strlen(pages[page][row]);}
float centre(int page,int row,int col){if(row!=4)return (10-keys(page,row))*.5f+col+.5f;float x=0;for(int c=0;c<col;c++)x+=widths[c];return x+widths[col]*.5f;}
void erase(std::string &s){if(s.empty())return;size_t n=s.size()-1;while(n&&(static_cast<unsigned char>(s[n])&0xc0)==0x80)--n;s.resize(n);}
}
void bindKeyboard(UI *u,SDL_Renderer *r,std::function<void()> tick){ui=u;renderer=r;service=std::move(tick);}
bool prosperoKeyboard(const std::string &title,std::string &value,bool password){
    if(!ui||!renderer)return false;
    std::string text=value;int page=0,row=1,col=0;PadState pad{};padUpdate(&pad);
    bool previousRecording=ui->recordingScreen();ui->recordScreen(false);SDL_StartTextInput();
    auto finish=[&](bool accepted){SDL_StopTextInput();ui->recordScreen(previousRecording);padUpdate(&pad);if(accepted)value=text;return accepted;};
    u64 repeated=0;Uint64 nextRepeat=0;
    for(;;){
        if(service)service();
        SDL_Event e;while(SDL_PollEvent(&e)){
            if(e.type==SDL_QUIT){SDL_PushEvent(&e);return finish(false);}
            if(e.type==SDL_TEXTINPUT && text.size()+std::strlen(e.text.text)<=1024)text+=e.text.text;
            if(e.type==SDL_KEYDOWN && e.key.keysym.sym==SDLK_BACKSPACE)erase(text);
        }
        padUpdate(&pad);u64 down=pad.down;auto now=SDL_GetTicks64();
        u64 direction=pad.held&(HidNpadButton_Up|HidNpadButton_Down|HidNpadButton_Left|HidNpadButton_Right);
        if(direction!=repeated){repeated=direction;nextRepeat=now+350;}else if(direction&&now>=nextRepeat){down|=direction;nextRepeat=now+85;}
        if(down&HidNpadButton_B)return finish(false);
        if(down&HidNpadButton_Plus)return finish(true);
        if(down&HidNpadButton_Y)erase(text);
        if((down&HidNpadButton_X)&&text.size()<1024)text+=' ';
        if(down&(HidNpadButton_L|HidNpadButton_R)){
            page=(page+((down&HidNpadButton_R)?1:2))%3;
            col=std::min(col,keys(page,row)-1);
        }
        if(down&(HidNpadButton_Up|HidNpadButton_Down)){
            int to=std::clamp(row+((down&HidNpadButton_Down)?1:-1),0,4);float x=centre(page,row,col);int best=0;
            for(int c=1;c<keys(page,to);c++)if(std::abs(centre(page,to,c)-x)<std::abs(centre(page,to,best)-x))best=c;
            row=to;col=best;
        }
        if(down&HidNpadButton_Left)col=std::max(0,col-1);
        if(down&HidNpadButton_Right)col=std::min(keys(page,row)-1,col+1);
        if(down&HidNpadButton_A){
            if(row<4){if(text.size()<1024)text+=pages[page][row][col];}
            else switch(col){case 0:page=page==0?1:0;break;case 1:page=page==2?0:2;break;case 2:if(text.size()<1024)text+=' ';break;case 3:erase(text);break;case 4:return finish(true);}
        }
        ui->begin();ui->text(80,52,"Prospero Hi-Fi",32,ACCENT);ui->text(80,105,title,26,TEXT,1100);
        ui->rect(80,153,1120,65,PANEL);
        std::string visible=password?std::string(std::min<size_t>(text.size(),80),'*'):text;
        if(visible.size()>85)visible="..."+visible.substr(visible.size()-82);
        ui->text(100,168,visible+((now/500)%2?"|":""),24,TEXT,1080);
        const char *labels[]={page==0?"ABC":"abc",page==2?"abc":"#+=","Space","Delete","Done"};
        for(int r=0;r<5;r++)for(int c=0;c<keys(page,r);c++){
            float kw=r==4?widths[c]:1.f;int x=140+int((centre(page,r,c)-kw*.5f)*100),w=int(kw*100)-10;
            std::string label=r==4?labels[c]:std::string(1,pages[page][r][c]);
            ui->button(x,246+r*89,w,73,label,r==row&&c==col);
        }
        ui->text(90,ui->layout().bottom(710),"Cross Type   Square Delete   Triangle Space   L1/R1 Page   Options Done   Circle Cancel",18,MUTED);
        SDL_RenderPresent(renderer);
#ifndef __PROSPERO__
        SDL_Delay(16);
#endif
    }
}
}
