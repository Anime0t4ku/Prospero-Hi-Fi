#include "storage_devices.hpp"
#ifdef __PROSPERO__
#include "OrbisDiscDrive.h"
#include <sys/ioctl.h>
#include <cam/cam.h>
#include <cam/cam_ccb.h>
#include <cam/scsi/scsi_pass.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#endif
namespace nxh {
bool usbOpticalDriveDetected(){
#ifdef __PROSPERO__
    for(int n=0;n<32;n++){
        auto path="/dev/pass"+std::to_string(n);
        int fd=open(path.c_str(),O_RDWR);if(fd<0)fd=open(path.c_str(),O_RDONLY);if(fd<0)continue;
        bool usb=OrbisDiscDrive::IsUsb(fd);
        unsigned char inquiry[36]{};
        bool optical=usb&&OrbisDiscDrive::Command(fd,orbis_mmc::Inquiry(),inquiry,sizeof(inquiry),500)&&(inquiry[0]&31)==5;
        close(fd);if(optical)return true;
    }
#endif
    return false;
}
}
