#include "storage_devices.hpp"
#include "disc.hpp"
#include "platform_paths.hpp"
#include <sys/stat.h>
#include <dirent.h>
#include <mutex>
#include <thread>
#include <condition_variable>
namespace nxh {
bool mountedStorageVolume(const std::string &path){
    auto parentPath=path.substr(0,path.find_last_of('/'));if(parentPath.empty())parentPath="/";
    struct stat st{},parent{};
    if(stat(path.c_str(),&st)!=0||!S_ISDIR(st.st_mode)||stat(parentPath.c_str(),&parent)!=0)return false;
    // Native applications must use the existing imported stat API. The payload
    // SDK's statfs wrapper embeds a raw syscall and cannot be called here.
    // Separate storage volumes have a different device ID from their parent;
    // a leftover mount directory stays on the parent's filesystem.
    if(st.st_dev==parent.st_dev)return false;
    auto *dir=opendir(path.c_str());if(!dir)return false;closedir(dir);return true;
}
namespace {
struct Detection {
    std::mutex mutex;std::condition_variable wake;bool quit=false;
    std::vector<StorageDevice> devices{{-1,"Internal Storage","/data"}};
    std::thread worker;
    Detection():worker([this]{
        for(;;){
            std::vector<std::string> mounts;
            for(auto path:{"/mnt/ext1","/mnt/ext0"})if(mountedStorageVolume(path))mounts.push_back(path);
            for(int i=0;i<8;i++){auto path="/mnt/usb"+std::to_string(i);if(mountedStorageVolume(path))mounts.push_back(path);}
            auto found=storageInventory(mounts,usbOpticalDriveDetected());
            std::unique_lock<std::mutex> lock(mutex);devices=std::move(found);
            if(wake.wait_for(lock,std::chrono::seconds(3),[this]{return quit;}))return;
        }
    }){}
    void stop(){ {std::lock_guard<std::mutex> lock(mutex);quit=true;}wake.notify_all();if(worker.joinable())worker.join(); }
    ~Detection(){stop();}
};
Detection &detection(){static Detection instance;return instance;}
}
std::vector<StorageDevice> storageInventory(const std::vector<std::string> &mounts,bool optical){
    std::vector<StorageDevice> found{{-1,"Internal Storage","/data"}};
    auto has=[&](const std::string &p){return std::find(mounts.begin(),mounts.end(),p)!=mounts.end();};
    if(has("/mnt/ext1"))found.push_back({-3,"M.2 Storage","/mnt/ext1"});
    if(has("/mnt/ext0"))found.push_back({-4,"USB Storage (Extended)","/mnt/ext0"});
    for(int i=0;i<8;i++){auto p="/mnt/usb"+std::to_string(i);if(has(p))found.push_back({-10-i,"USB Storage "+std::to_string(i+1),p});}
    if(optical)found.push_back({-2,"Physical Disc",DiscRoot});return found;
}
std::vector<StorageDevice> detectedStorageDevices(){auto &d=detection();std::lock_guard<std::mutex> lock(d.mutex);return d.devices;}
void stopStorageDetection(){detection().stop();}
std::string storageFolder(const Settings &settings,const StorageDevice &device){
    if(device.id==-1)return settings.musicRoot;
    if(device.id==-2)return DiscRoot;
    auto it=settings.storageFolders.find(device.mount);
    return it==settings.storageFolders.end()?device.mount+"/Music":it->second;
}
bool resolveStorageSource(const Settings &settings,int id,Source &source){
    if(id==-1){source={false,{},settings.musicRoot};return true;}
    if(id==-2){source={false,{},DiscRoot};return true;}
    for(auto &device:detectedStorageDevices())if(device.id==id){source={false,{},storageFolder(settings,device)};return true;}
    return false;
}
}
