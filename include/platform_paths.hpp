#pragma once
#include <string>
namespace nxh {
inline bool validLocalRoot(const std::string &s) {
    if(s.empty() || s.front()!='/' || s.find('\\')!=std::string::npos || s.find("/../")!=std::string::npos || s.find("/./")!=std::string::npos || s.size()>=3 && s.compare(s.size()-3,3,"/..") == 0) return false;
    if(s=="/data" || s.rfind("/data/",0)==0 || s=="/download0" || s.rfind("/download0/",0)==0) return true;
    if(s.size()>=9 && s.rfind("/mnt/ext",0)==0 && (s[8]=='0'||s[8]=='1') && (s.size()==9||s[9]=='/'))return true;
    return s.size()>=9 && s.rfind("/mnt/usb",0)==0 && s[8]>='0' && s[8]<='7' && (s.size()==9 || s[9]=='/');
}
inline bool validStorageFolder(const std::string &mount,const std::string &folder){
    bool drive=mount=="/mnt/ext0"||mount=="/mnt/ext1"||(mount.size()==9&&mount.rfind("/mnt/usb",0)==0&&mount[8]>='0'&&mount[8]<='7');
    return drive && validLocalRoot(folder) && folder.size()<=1024 && folder.find('\0')==std::string::npos &&
           (folder==mount||folder.rfind(mount+"/",0)==0);
}
inline const char *configPath() { return "/data/prospero-hifi/config.json"; }
}
