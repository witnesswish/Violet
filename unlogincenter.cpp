#include "unlogincenter.h"

UnloginCenter::UnloginCenter() {
}

void UnloginCenter::sendBordcast(int from, std::string content) {
    std::cout<< "sb: " << content <<std::endl;
    for(auto& fd : onlineUnlogin)
    {
        SRHelper sr;
        VioletProtNeck neck = {};
        strcpy(neck.command, "nongb");
        neck.mfrom = from;
        sr.sendMsg(fd, neck, content);
    }
}

void UnloginCenter::addNewUnlogin(int fd) {
    // 这里后续可以做别的逻辑判断
    for(auto& afd : onlineUnlogin)
    {
        if(afd == fd)
        {
            return;
        }
    }
    onlineUnlogin.push_back(fd);
}

void UnloginCenter::removeUnlogin(int fd) {
    onlineUnlogin.remove_if([=](int x){return x = fd;});
}
