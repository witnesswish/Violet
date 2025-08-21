#include <iostream>

#include "unlogincenter.h"

std::list<int> UnloginCenter::onlineUnlogin = {};

UnloginCenter::UnloginCenter()
{
}

void UnloginCenter::sendBordcast(int from, std::string content)
{
    std::cout << "sb: " << content << std::endl;
    for (auto &fd : onlineUnlogin)
    {
        if (fd == from)
        {
            continue;
        }
        SRHelper sr;
        VioletProtNeck neck = {};
        strcpy(neck.command, "nongb");
        strcpy(neck.name, std::to_string(from).c_str());
        neck.mfrom = from;
        sr.sendMsg(fd, neck, content);
    }
}

void UnloginCenter::addNewUnlogin(int fd)
{
    // 这里后续可以做别的逻辑判断
    for (auto &afd : onlineUnlogin)
    {
        if (afd == fd)
        {
            std::cout << "user #" << fd << "already on list" << std::endl;
            return;
        }
    }
    onlineUnlogin.push_back(fd);
    VioletProtNeck neck = {};
    strcpy(neck.command, "nonigsucc");
    std::string tmp("violet");
    sr.sendMsg(fd, neck, tmp);
    std::cout << "there is " << onlineUnlogin.size() << " on lists" << std::endl;
}

void UnloginCenter::removeUnlogin(int fd)
{
    onlineUnlogin.remove_if([fd](int x){ return x == fd; });
    std::cout << "remove #" << fd << " remain: " << onlineUnlogin.size() << std::endl;
}
void UnloginCenter::privateChate(int fd, uint8_t mto, std::string &text)
{
    VioletProtNeck neck = {};
    for (auto &afd : onlineUnlogin)
    {
        if (afd == mto)
        {
            strcpy(neck.name, std::to_string(fd).c_str());
            strcpy(neck.command, (const char *)"nonpb");
            neck.mfrom = fd;
            sr.sendMsg(mto, neck, text);
            return;
        }
    }
    strcpy(neck.command, (const char *)"nonperr");
    strcpy(neck.name, std::to_string(mto).c_str());
    std::string tmp("user not online");
    sr.sendMsg(fd, neck, tmp);
}
