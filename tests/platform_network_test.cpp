#include "platform.hpp"
#include <cassert>
#include <cstring>
#include <iostream>
namespace {const char *reported="192.168.1.23";int result=0;}
extern "C" int sceNetCtlGetInfo(int code,void *info){assert(code==14);std::strcpy(static_cast<char*>(info),reported);return result;}
extern "C" int sceNetCtlInit(){return 0;}
extern "C" int sceNetInit(){return 0;}
extern "C" int ProsperoSDLFollowOutput(){return 0;}
namespace nxh {bool ensureStorageAccess(){return false;}}
int main(){
    assert(nxh::localNetworkAddress()=="192.168.1.23");
    reported="10.0.0.5";assert(nxh::localNetworkAddress()==reported);
    for(auto invalid:{"0.0.0.0","127.0.0.1","255.255.255.255","invalid",""}){reported=invalid;assert(nxh::localNetworkAddress().empty());}
    reported="192.168.1.23";result=-1;assert(nxh::localNetworkAddress().empty());
    std::cout<<"PS5 address query: valid LAN IP, invalid/loopback/zero addresses and API failure passed.\n";
}
