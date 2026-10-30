#include "disc.hpp"
#include "disc_input.hpp"
#include <cstring>
#include <chrono>
#include <fstream>
#include "cover_image.hpp"
#include "platform_paths.hpp"
#include "platform.hpp"
#ifdef __PROSPERO__
#include "OrbisDiscDrive.h"
#endif
#include <curl/curl.h>
#include <fcntl.h>
#include <unistd.h>
namespace nxh {
namespace {
std::mutex deviceMutex, catalogMutex;
struct Album { DiscTOC toc; DiscMetadata meta; };
std::map<std::string,Album> catalog;
#ifdef __PROSPERO__
struct Drive {
    int pass=-1;
    std::string path;
    std::mutex io;
    ~Drive(){if(pass>=0)close(pass);}
    bool select(std::string &error) {
        // USB units may expose only pass(4), so enumerate both paths. Never assume cd0 is USB.
        for(const auto &device:OrbisDiscDrive::List()){
            int cd=open(device.c_str(),O_RDONLY);if(cd<0)continue;
            int candidate=OrbisDiscDrive::OpenPassThrough(cd);close(cd);
            if(candidate>=0 && tryDrive(candidate,device))return true;
        }
        for(int n=0;n<32;n++){
            std::string device="/dev/pass"+std::to_string(n);
            int candidate=open(device.c_str(),O_RDWR);if(candidate<0)candidate=open(device.c_str(),O_RDONLY);
            if(candidate>=0 && tryDrive(candidate,device))return true;
        }
        error="No readable audio CD found. Insert a CD in a powered USB CD/DVD drive and reopen Physical Disc.";return false;
    }
    bool tryDrive(int candidate,const std::string &device) {
        unsigned char inquiry[36]{};
        bool optical=OrbisDiscDrive::IsUsb(candidate) && OrbisDiscDrive::Command(candidate,orbis_mmc::Inquiry(),inquiry,sizeof inquiry,4000) && (inquiry[0]&31)==5;
        if(optical){
            int previous=pass;pass=candidate;DiscTOC t;std::string error;
            if(tocUnlocked(t,error)){if(previous>=0)close(previous);path=device;return true;}
            pass=previous;
        }
        close(candidate);return false;
    }
    bool tocUnlocked(DiscTOC &t,std::string &error){
        if(pass<0){error="No USB audio CD drive is open.";return false;}
        if(!OrbisDiscDrive::Ready(pass) && !OrbisDiscDrive::Ready(pass)){error="Drive not ready or disc changed.";return false;}
        // Read the 4-byte header first, then exactly the advertised TOC length.
        unsigned char header[4]{};
        std::vector<uint8_t> cdb={0x43,2,0,0,0,0,0,0,4,0};
        if(!OrbisDiscDrive::Command(pass,cdb,header,4,5000)){error="Could not read CD TOC header.";return false;}
        unsigned size=(unsigned(header[0])<<8|header[1])+2;
        if(size<12||size>804){error="Invalid CD table of contents.";return false;}
        std::vector<unsigned char> raw(size);cdb[7]=size>>8;cdb[8]=size;
        if(!OrbisDiscDrive::Command(pass,cdb,raw.data(),size,5000)){error="Could not read CD table of contents.";return false;}
        return parseDiscTOC(raw.data(),raw.size(),t,error);
    }
    bool toc(DiscTOC &t,std::string &error){std::lock_guard<std::mutex> lock(io);if(pass<0&&!select(error))return false;return tocUnlocked(t,error);}
    bool read(int lba,int count,unsigned char *out){
        std::lock_guard<std::mutex> lock(io);
        if(pass<0||!OrbisDiscDrive::Ready(pass))return false;
        while(count>0){
            int n=std::min(count,16);std::vector<uint8_t> cdb(12,0);cdb[0]=0xBE;cdb[1]=0x04;
            orbis_mmc::Be32(&cdb[2],uint32_t(lba));cdb[8]=n;cdb[9]=0x10;
            if(!OrbisDiscDrive::Command(pass,cdb,out,n*2352,5000))return false;
            lba+=n;count-=n;out+=n*2352;
        }return true;
    }
};
#else
struct Drive {
    bool toc(DiscTOC &,std::string &error){error="USB CD access is available in the PS5 build.";return false;}
    bool read(int,int,unsigned char*){return false;}
};
#endif
std::shared_ptr<Drive> drive;
std::shared_ptr<Drive> getDrive(bool refresh=false) {
    std::lock_guard<std::mutex> lock(deviceMutex);
    // Never invalidate a live input. Recreate between scans to handle unplug/replug.
    if(refresh&&drive&&drive.use_count()==1)drive.reset();
    if(!drive)drive=std::make_shared<Drive>();
    return drive;
}
struct Download { std::vector<unsigned char> *bytes;size_t limit; };
size_t receive(void *p,size_t size,size_t n,void *user) {
    auto &d=*static_cast<Download*>(user);
    if(size&&n>SIZE_MAX/size)return 0;
    size_t count=size*n;
    if(count>d.limit-d.bytes->size())return 0;
    auto *b=static_cast<unsigned char*>(p);d.bytes->insert(d.bytes->end(),b,b+count);return count;
}
DiscHTTP http() {
    static std::once_flag init;
    std::call_once(init,[]{curl_global_init(CURL_GLOBAL_DEFAULT);});
    auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(12);
    return [deadline](const std::string &url,std::vector<unsigned char> &out,size_t limit) {
        out.clear();
        long remaining=std::chrono::duration_cast<std::chrono::milliseconds>(deadline-std::chrono::steady_clock::now()).count();
        if(remaining<=0)return 0;
        CURL *c=curl_easy_init();if(!c)return 0;
        Download data{&out,limit};
        curl_easy_setopt(c,CURLOPT_URL,url.c_str());
        curl_easy_setopt(c,CURLOPT_USERAGENT,"Prospero-Hi-Fi/1.0.0 (NX-Hi-Fi port)");
        curl_easy_setopt(c,CURLOPT_FOLLOWLOCATION,1L);
        curl_easy_setopt(c,CURLOPT_MAXREDIRS,5L);
        curl_easy_setopt(c,CURLOPT_PROTOCOLS,CURLPROTO_HTTPS);
        curl_easy_setopt(c,CURLOPT_REDIR_PROTOCOLS,CURLPROTO_HTTPS);
        curl_easy_setopt(c,CURLOPT_NOSIGNAL,1L);
        curl_easy_setopt(c,CURLOPT_CONNECTTIMEOUT_MS,std::min(remaining,4000L));
        curl_easy_setopt(c,CURLOPT_TIMEOUT_MS,std::min(remaining,8000L));
        std::string ca=std::string(assetRoot())+"cacert.pem";
        curl_easy_setopt(c,CURLOPT_CAINFO,ca.c_str());
        curl_easy_setopt(c,CURLOPT_WRITEFUNCTION,receive);
        curl_easy_setopt(c,CURLOPT_WRITEDATA,&data);
        auto result=curl_easy_perform(c);long status=0;
        if(result==CURLE_OK)curl_easy_getinfo(c,CURLINFO_RESPONSE_CODE,&status);
        curl_easy_cleanup(c);return int(status);
    };
}

bool pathTrack(const std::string &path,std::string &id,int &number) {
    auto slash=path.find('/');if(slash!=28||path.size()!=40)return false;
    id=path.substr(0,slash);
    if(id.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789._-")!=std::string::npos)return false;
    auto name=path.substr(slash+1);
    if(name.substr(0,5)!="Track"||name.substr(7)!=".wav"||!std::isdigit(name[5])||!std::isdigit(name[6]))return false;
    number=(name[5]-'0')*10+name[6]-'0';return number>=1&&number<=99;
}
} // namespace
bool listDisc(std::vector<Entry> &out,std::string &error) {
    out.clear();
    auto d=getDrive(true);DiscTOC toc;
    if(!d->toc(toc,error))return false;
    auto id=discID(toc);
    auto meta=lookupDiscMetadata(id,"/data/prospero-hifi/cd-cache",http(),[](const auto &bytes) {
        auto *surface=boundedCoverSurface(bytes);
        if(!surface)return false;
        SDL_FreeSurface(surface);return true;
    });
    // Check again after network lookup; do not list a disc that was swapped meanwhile.
    DiscTOC check;
    if(!d->toc(check,error)||discID(check)!=id){error="Disc changed. Open Physical Disc again.";return false;}
    for(auto &t:toc.audio) {
        char filename[32];std::snprintf(filename,sizeof filename,"Track%02d.wav",t.number);
        char label[32];std::snprintf(label,sizeof label,"Track %02d",t.number);
        auto title=meta.titles[t.number];if(title.empty())title=label;
        out.push_back({title,id+"/"+filename,false});
    }
    std::lock_guard<std::mutex> lock(catalogMutex);
    catalog[id]={std::move(toc),std::move(meta)};
    // Keep only a few discs in memory; the persistent cache retains all of them.
    while(catalog.size()>8){auto it=catalog.begin();if(it->first==id)++it;catalog.erase(it);}
    return true;
}
std::unique_ptr<Input> openDisc(const std::string &path,std::string &error,const std::atomic<uint64_t> *g,uint64_t e) {
    std::string id;int number;
    if(!pathTrack(path,id,number)){error="Invalid audio CD track.";return {};}
    auto d=getDrive();DiscTOC toc;
    if(!d->toc(toc,error))return {};
    if(discID(toc)!=id){error="Disc changed. Open Physical Disc again.";return {};}
    for(auto &t:toc.audio)if(t.number==number)return std::make_unique<CDDAInput>([d](int lba,int n,unsigned char *out){ return d->read(lba,n,out); },t,g,e);
    error="Audio CD track no longer exists.";
    return {};
}
TrackDetails discDetails(const std::string &path) {
    TrackDetails details;details.format="CDDA";details.sampleRate=44100;
    details.channels=2;details.bitDepth=16;details.bitRate=1411;details.album="Audio CD";
    std::string id;int number;if(!pathTrack(path,id,number))return details;
    char label[32];std::snprintf(label,sizeof label,"Track %02d",number);details.title=label;
    std::lock_guard<std::mutex> lock(catalogMutex);
    auto it=catalog.find(id);if(it==catalog.end())return details;
    auto &a=it->second;
    for(auto &t:a.toc.audio)if(t.number==number)details.duration=(t.end-t.start)/75.;
    if(a.meta.found) {
        details.album=a.meta.album;details.artist=a.meta.artist;
        auto title=a.meta.titles.find(number);if(title!=a.meta.titles.end()&&!title->second.empty())details.title=title->second;
        auto artist=a.meta.artists.find(number);if(artist!=a.meta.artists.end()&&!artist->second.empty())details.artist=artist->second;
    }
    return details;
}
std::vector<unsigned char> discCover(const std::string &path) {
    std::string id;int number;if(!pathTrack(path,id,number))return {};
    std::lock_guard<std::mutex> lock(catalogMutex);
    auto it=catalog.find(id);return it==catalog.end()?std::vector<unsigned char>{}:it->second.meta.cover;
}
void shutdownDisc() {
    { std::lock_guard<std::mutex> lock(deviceMutex);drive.reset(); }
    std::lock_guard<std::mutex> lock(catalogMutex);catalog.clear();
}
} // namespace nxh
