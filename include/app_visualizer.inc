// Local fullscreen visualizer and quick settings panel.
nlohmann::json remoteSettingFields()const{
    auto fields=settingFields();auto result=nlohmann::json::array();
    for(auto &f:fields)if(f["key"].template get<std::string>().rfind("visualizer_",0)!=0)result.push_back(f);
    return result;
}
Visualizer visualizer;
bool visualizerFullscreen=false,playerSettingsOpen=false;
int playerSettingsTab=0,playerSettingsRow=-1;
PlayerRect visualizerRect()const{return {playerX(),370,playerWidth(),std::max(8,seekRect().y-380)};}
bool canExpandVisualizer()const{return fullscreen&&settings.visualizer&&settings.visualizerExpand&&!listening;}
void visualizerChoice(const std::string &key){
    for(auto &f:settingFields())if(f["key"]==key){
        std::vector<nlohmann::json> values;std::vector<std::string> labels;
        for(auto &v:f["choices"]){values.push_back(v);labels.push_back(key=="visualizer_style"?visualizerNames[v.template get<int>()]:std::to_string(v.template get<int>())+"%");}
        choose(f["label"],values,labels,settingsDocument(settings)[key],[this,key](auto v){applySettings({{key,v}});});return;
    }
}
void playerSettingsChange(int dir=0){
    if(playerSettingsRow<0){if(dir)playerSettingsTab=1-playerSettingsTab;else playerSettingsRow=0;return;}
    if(playerSettingsTab==1){
        switch(playerSettingsRow){
        case 0:applySettings({{"visualizer",!settings.visualizer}});break;
        case 1:visualizerChoice("visualizer_style");break;
        case 2:visualizerChoice("visualizer_sensitivity");break;
        case 3:visualizerChoice("visualizer_smoothing");break;
        case 4:applySettings({{"visualizer_expand",!settings.visualizerExpand}});break;
        }return;
    }
    if(playerSettingsRow==0){settings.eqEnabled=!settings.eqEnabled;persist();}
    else if(playerSettingsRow==1){
        std::vector<nlohmann::json> values{-1};std::vector<std::string> labels{"Off"};int current=-1;
        for(int i=0;i<int(presets.size());i++){values.push_back(i);labels.push_back(presets[i].name);if(settings.eqEnabled&&settings.eq==presets[i].gains)current=i;}
        choose("Equalizer preset",values,labels,current,[this](auto v){int i=v.template get<int>();settings.eqEnabled=i>=0;if(i>=0)settings.eq=presets[i].gains;persist();});
    }else if(playerSettingsRow<7){auto &gain=settings.eq[playerSettingsRow-2];gain=std::clamp(gain+(dir?dir:1),-6.f,6.f);settings.eqEnabled=true;persist();}
    else{settings.eq={};persist();}
}
void drawPlayerSettings(){
    if(!playerSettingsOpen)return;
    ui.roundedRect(140,100,1000,510+layout().extraHeight(),PANEL,18);
    ui.button(168,118,454,46,"Equalizer",playerSettingsTab==0);
    ui.button(638,118,474,46,"Visualizer",playerSettingsTab==1);
    if(playerSettingsRow==-1)ui.text(170,171,"Left / Right: switch tabs    Cross: enter",14,ACCENT,930);
    std::vector<std::string> names,values;
    if(playerSettingsTab==0){
        names={"Equalizer","Preset","Bass 60 Hz","Low-mid 250 Hz","Mid 1 kHz","High-mid 4 kHz","Treble 12 kHz","Reset gains"};
        std::string preset="Custom";for(auto &p:presets)if(settings.eq==p.gains)preset=p.name;
        values={settings.eqEnabled?"On":"Off",preset};for(auto gain:settings.eq)values.push_back(std::to_string(int(gain))+" dB");values.push_back("");
    }else{
        names={"Show visualizer","Fullscreen style","Sensitivity","Smoothing","Select to expand"};
        values={settings.visualizer?"On":"Off",visualizerNames[settings.visualizerStyle],std::to_string(settings.visualizerSensitivity)+"%",std::to_string(settings.visualizerSmoothing)+"%",settings.visualizerExpand?"On":"Off"};
    }
    for(int i=0;i<int(names.size());i++){
        int y=198+i*43;ui.button(170,y,940,39,"",playerSettingsRow==i);ui.text(188,y+9,names[i],17,TEXT,420);ui.rightText(1088,y+9,values[i],17,ACCENT);
    }
    ui.text(170,565+layout().extraHeight(),"Up: tabs    Left / Right: adjust    Cross: choose    Circle: close",15,MUTED,940);
}
void drawLocalVisualizer(const Playback &p){
    // The remote frame has stock bars and the normal player, even during expansion.
    if(!fullscreen||!settings.visualizer)return;
    bool recording=ui.recordingScreen();ui.recordScreen(false);
    if(visualizerFullscreen){
        visualizer.draw(ui,p,{24,24,1232,layout().height()-48},settings);
    }else{
        auto a=visualizerRect();visualizer.draw(ui,p,a,settings);
        if(canExpandVisualizer()&&fullFocus.row==-1&&!playerSettingsOpen&&!optionApply){
            ui.rect(a.x,a.y,a.w,2,ACCENT);ui.rect(a.x,a.y+a.h-2,a.w,2,ACCENT);
            ui.text(a.x+8,a.y+6,"Cross: expand",13,MUTED,a.w-16);
        }
    }
    ui.recordScreen(recording);
}
