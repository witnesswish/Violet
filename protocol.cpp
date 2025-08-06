#include "protocol.h"


SRHelper::SRHelper() {}

void SRHelper::sendMsg(int fd, uint16_t msgType, const std::string &content) {
    Msg msg;
    msg.header.magic = htonl(0x43484154); // "CHAT"
    msg.header.version = htons(1);
    msg.header.type = htons(msgType);
    msg.header.length = htonl(content.size());
    msg.header.timestamp = htonl(static_cast<uint32_t>(time(nullptr)));
    msg.content.assign(content.begin(), content.end());
    auto packet = msg.serialize();
    if(send(fd, packet.data(), packet.size(), 0) < 0)
    {
        perror("send msg error");
    }
    else
    {
        std::cout<< "send msg success" <<std::endl;
    }
}

void SRHelper::sendMsg(int fd, VioletProtHeader header, std::string &content) {
    Msg msg;
    msg.header = header;
    msg.content.assign(content.begin(), content.end());
    auto packet = msg.serialize();
    if(send(fd, packet.data(), packet.size(), 0) < 0)
    {
        perror("send msg error");
    }
    else
    {
        std::cout<< "send msg success" <<std::endl;
    }
}

void SRHelper::sendMsg(int fd, VioletProtNeck neck, std::string &content) {
    Msg msg;
    msg.header.magic = htonl(0x43484154); // "CHAT"
    msg.header.version = htons(1);
    msg.header.type = htons(0);
    msg.header.length = htonl(content.size());
    msg.header.timestamp = htonl(static_cast<uint32_t>(time(nullptr)));
    msg.neck = neck;
    msg.content.assign(content.begin(), content.end());
    auto packet = msg.serialize();
    if(send(fd, packet.data(), packet.size(), 0) < 0)
    {
        perror("send msg error");
    }
    else
    {
        std::cout<< "send msg success" <<std::endl;
    }
}

void SRHelper::sendMsg(int fd, VioletProtHeader header, VioletProtNeck neck, std::string &content)
{
    Msg msg;
    msg.header = header;
    msg.neck = neck;
    msg.content.assign(content.begin(), content.end());
    auto packet = msg.serialize();
    if(send(fd, packet.data(), packet.size(), 0) < 0)
    {
        perror("send msg error");
    }
    else
    {
        std::cout<< "send msg success" <<std::endl;
    }
}

std::optional<Msg> SRHelper::recvMsg(int fd) {
    VioletProtHeader header;
    ssize_t len = recv(fd, &header, sizeof(header), MSG_PEEK);
    if(len == 0)
    {
        close(fd);
        std::cout<< "client #" << fd << " closed" <<std::endl;
        return std::nullopt;
    }
    if(len < 0)
    {
        if(errno != 104)
        {
            close(fd);
            std::cout<< "client #" << fd << " crushed, close it" <<std::endl;
        }
        else
        {
            std::cout<< "recv error: " << errno <<std::endl;
        }
        return std::nullopt;
    }
    if(len != sizeof(header))
    {
        std::cout<< "recv error number, not match header size at first" <<std::endl;
        return std::nullopt;
    }
    uint32_t contentLen = ntohl(header.length);
    uint32_t totaLen = contentLen + sizeof(header);
    std::vector<char> recvBuffer(totaLen);
    len = recv(fd, recvBuffer.data(), recvBuffer.size(), 0);
    if(len != static_cast<ssize_t>(recvBuffer.size()))
    {
        return std::nullopt;
    }
    //这个部分返回的是vector<char>,转为string使用构造函数std::string str(ret.data(), ret.size())
    return Msg::deserialize(recvBuffer.data(), recvBuffer.size());
}
