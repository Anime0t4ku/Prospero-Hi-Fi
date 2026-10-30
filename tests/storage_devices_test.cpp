#include "storage_devices.hpp"
#include "platform_paths.hpp"
#include "disc.hpp"
#include <cassert>
#include <cstdio>
#include <unistd.h>
#include <sys/stat.h>
#include <fstream>
#include <iostream>
using namespace nxh;
int main(){
    char temporary[]="/tmp/prospero-mount-test-XXXXXX";auto *root=mkdtemp(temporary);assert(root);
    auto leftover=std::string(root)+"/usb0";assert(mkdir(leftover.c_str(),0700)==0);
    assert(!mountedStorageVolume(leftover));
    {std::ofstream f(leftover+"/track.mp3");f<<"not a real mounted drive";}
    assert(!mountedStorageVolume(leftover));assert(!mountedStorageVolume(leftover+"/missing"));
#ifdef __linux__
    assert(mountedStorageVolume("/proc"));
#endif
    std::remove((leftover+"/track.mp3").c_str());rmdir(leftover.c_str());rmdir(root);
    auto absent=storageInventory({},false);assert(absent.size()==1&&absent[0].id==-1);
    auto drives=storageInventory({"/mnt/usb7","/mnt/ext1","/mnt/usb0","/mnt/usb0","/mnt/fake"},true);
    assert(drives.size()==5);assert(drives[1].name=="M.2 Storage");assert(drives[2].id==-10&&drives[3].id==-17&&drives[4].id==-2);
    Settings s;s.musicRoot="/data/music";
    assert(storageFolder(s,drives[1])=="/mnt/ext1/Music");assert(storageFolder(s,drives[2])=="/mnt/usb0/Music");
    std::string error;
    assert(patchSettings(s,{{"storage_folders",{{"/mnt/ext1","/mnt/ext1/Albums"},{"/mnt/usb0","/mnt/usb0/My Music"}}}},error));
    assert(storageFolder(s,drives[1])=="/mnt/ext1/Albums");
    auto before=s.storageFolders;
    for(auto folder:{"/data/music","/mnt/usb0/../ext1/Music","/mnt/usb01/Music","/mnt/usb0/.."}){
        assert(!patchSettings(s,{{"storage_folders",{{"/mnt/usb0",folder}}}},error));assert(s.storageFolders==before);
    }
    auto cfg="/tmp/prospero-storage-test-"+std::to_string(getpid())+".json";
    assert(saveSettings(cfg,s,error));Settings loaded;assert(loadSettings(cfg,loaded,error));assert(loaded.storageFolders==s.storageFolders);
    assert(storageFolder(loaded,drives[2])=="/mnt/usb0/My Music");
    assert(storageInventory({},false)==absent); // Removing all drives hides external/disc entries.
    assert(storageInventory({},true).size()==2); // Optical drive entry doesn't require a disc/TOC.
    s.musicRoot="/mnt/usb3/Audio";assert(saveSettings(cfg,s,error));Settings migrated;
    assert(loadSettings(cfg,migrated,error));assert(migrated.musicRoot=="/data/music"&&migrated.storageFolders.at("/mnt/usb3")=="/mnt/usb3/Audio");
    std::remove(cfg.c_str());std::remove((cfg+".bak").c_str());
    assert(validLocalRoot("/mnt/ext1/Music")&&!validLocalRoot("/mnt/ext2/Music"));
    std::cout<<"Drive visibility, stable IDs, Music defaults, custom-folder persistence, validation, removal and legacy migration passed.\n";
}
