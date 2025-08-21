#include "protocol.h"
#include <limits.h>
#include <sys/types.h>
#include <sys/ioctl.h>

SRHelper::SRHelper() {}

void SRHelper::sendMsg(int fd, uint16_t msgType, const std::string &content)
{
    Msg msg;
    msg.header.magic = htonl(0x43484154); // "CHAT"
    msg.header.version = htons(1);
    msg.header.type = htons(msgType);
    msg.header.length = htonl(content.size());
    msg.header.timestamp = htonl(static_cast<uint32_t>(time(nullptr)));
    msg.neck = {};
    msg.content.assign(content.begin(), content.end());
    auto packet = msg.serialize();
    if (send(fd, packet.data(), packet.size(), 0) < 0)
    {
        perror("send msg error");
    }
    else
    {
        std::cout << "send msg success" << std::endl;
    }
}

void SRHelper::sendMsg(int fd, VioletProtHeader header, std::string &content)
{
    Msg msg;
    msg.header = header;
    msg.content.assign(content.begin(), content.end());
    auto packet = msg.serialize();
    if (send(fd, packet.data(), packet.size(), 0) < 0)
    {
        perror("send msg error");
    }
    else
    {
        std::cout << "send msg success" << std::endl;
    }
}

void SRHelper::sendMsg(int fd, VioletProtNeck neck, std::string &content)
{
    Msg msg;
    msg.header.magic = htonl(0x43484154); // "CHAT"
    msg.header.version = htons(1);
    msg.header.type = htons(0);
    msg.header.length = htonl(content.size());
    msg.header.timestamp = htonl(static_cast<uint32_t>(time(nullptr)));
    msg.neck = neck;
    msg.content.assign(content.begin(), content.end());
    auto packet = msg.serialize();
    if (send(fd, packet.data(), packet.size(), 0) < 0)
    {
        perror("send msg error");
    }
    else
    {
        std::cout << "send msg success" << std::endl;
    }
}

void SRHelper::sendMsg(int fd, VioletProtHeader header, VioletProtNeck neck, std::string &content)
{
    Msg msg;
    msg.header = header;
    msg.neck = neck;
    msg.content.assign(content.begin(), content.end());
    auto packet = msg.serialize();
    if (send(fd, packet.data(), packet.size(), 0) < 0)
    {
        perror("send msg error");
    }
    else
    {
        std::cout << "send msg success" << std::endl;
    }
}

Msg::Msg()
{
    //content.reserve(8000);
}

std::vector<char> Msg::serialize() const
{
    std::vector<char> packet(sizeof(VioletProtHeader) + sizeof(VioletProtNeck) + content.size());
    memcpy(packet.data(), &header, sizeof(header));
    memcpy(packet.data() + sizeof(VioletProtHeader), &neck, sizeof(neck));
    if (!content.empty())
    {
        memcpy(packet.data() + sizeof(VioletProtHeader) + sizeof(VioletProtNeck), content.data(), content.size());
    }
    return packet;
}

std::optional<Msg> Msg::deserialize(const char *data, ssize_t length)
{
    std::cout<< "deserialize length: " << length <<std::endl;
    if (length < sizeof(VioletProtHeader))
        return std::nullopt;
    Msg msg;
    memcpy(&msg.header, data, sizeof(msg.header));
    memcpy(&msg.neck, data + sizeof(msg.header), sizeof(msg.neck));
    if (ntohl(msg.header.magic) != 0x43484154)
    {
        std::cout<< "header magic bnumber error, return nullopt" <<std::endl;
        return std::nullopt;
    }
    size_t excepted_len = sizeof(msg.header) + sizeof(msg.neck) + ntohl(msg.header.length);
    size_t body_len = length - sizeof(msg.header) - sizeof(msg.neck);
    std::cout<< "deserialize body_len: " << body_len << " actuall bodylen: " << ntohl(msg.header.length) <<std::endl;
    //size_t body_len = ntohl(msg.header.length);
    // if (length < excepted_len)
    // {
    //     return std::nullopt;
    // }
    if (body_len > 0)
    {
        msg.content.assign(data + sizeof(msg.header) + sizeof(msg.neck), data + sizeof(msg.header) + sizeof(msg.neck) + body_len);
    }
    msg.header.checksum = static_cast<uint32_t>(length) - sizeof(msg.neck) - sizeof(msg.header);
    std::cout<< "deserialize msg header length: " << (msg.header.length) <<std::endl;
    return msg;
}

std::optional<Msg> SRHelper::recvMsg(int fd, ssize_t byteToRead)
{
    VioletProtHeader header;
    VioletProtNeck neck;
    int byteReadable;
    if(ioctl(fd, FIONREAD, &byteReadable) < 0)
    {
        perror("ioctl(fionread) error");
    }
    std::cout<< "byte readable in socket: " << byteReadable <<std::endl;
    if(byteToRead > 0)
    {
        Msg spByteRead;
        std::vector<char> recvBuffer(byteToRead);
        ssize_t totalRead = 0;
        ssize_t len = recv(fd, recvBuffer.data(), recvBuffer.size(), 0);
        totalRead = len;
        if(len < 0)
        {
            std::cout << "read " << byteToRead << " failure, return nullopt" <<std::endl;
            return std::nullopt;
        }
        else if(len == 0)
        {
            std::cout << "read 0, goint to kinck user out" <<std::endl;
            Msg msg = {};
            msg.header.length = 0;
            msg.header.checksum = 0;
            return msg;
        }
        else if(len < byteToRead)
        {
            ssize_t tmp = byteToRead - len;
            while(tmp > 0)
            {
                spByteRead.content.assign(recvBuffer.begin(), recvBuffer.begin()+totalRead);
                len = recv(fd, recvBuffer.data()+totalRead, recvBuffer.size()-totalRead, 0);
                if(len < 0)
                {
                    break;
                }
                else if(len == 0)
                {
                    Msg msg = {};
                    msg.header.length = 0;
                    msg.header.checksum = 0;
                    return msg;
                }
                else
                {
                    tmp = tmp - len;
                    totalRead += len;
                    spByteRead.content.insert(spByteRead.content.end(), recvBuffer.begin(), recvBuffer.begin()+len);
                }
            }
            std::cout<< "read special byte complete after while, read: " << totalRead <<std::endl;
            spByteRead.header.checksum = totalRead;
            return spByteRead;
        }
        else
        {
            std::cout<< "read special byte complete, read: " << totalRead <<std::endl;
            spByteRead.header.checksum = totalRead;
            spByteRead.content.assign(recvBuffer.begin(), recvBuffer.begin()+totalRead);
        }
        return spByteRead;
    }
    ssize_t len = recv(fd, &header, sizeof(header), MSG_PEEK);
    std::cout << "peek len: " << len << " of recv" << std::endl;
    if (len == 0)
    {
        close(fd);
        std::cout << "client #" << fd << " closed" << std::endl;
        Msg msg = {};
        // 这里我的想法是找一个字段，用来存状态，但是先用这个吧
        msg.header.length = 0;
        msg.header.checksum = 0;
        return msg;
    }
    if (len < 0)
    {
        if (errno == 104)
        {
            close(fd);
            std::cout << "client #" << fd << " crushed, close it" << std::endl;
            Msg msg = {};
            msg.header.length = 0;
            msg.header.checksum = 0;
            return msg;
        }
        else if (errno == 11)
        {
            Msg msg = {};
            std::cout << "read on nonblock" << std::endl;
            msg.header.length = htonl(1);
            msg.header.checksum = 1;
            return msg;
        }
        else
        {
            std::cout << "recv error: " << errno << std::endl;
        }
        return std::nullopt;
    }
    if (len != sizeof(header))
    {
        std::cout << "recv error number, not match header size at first" << std::endl;
        return std::nullopt;
    }
    uint32_t contentLen = ntohl(header.length);
    std::cout << "content len to malloc " << contentLen << " of recv" << std::endl;
    uint32_t totaLen = contentLen + sizeof(header) + sizeof(neck);
    std::cout << "total len to malloc " << totaLen << " of recv" << std::endl;
    std::vector<char> recvBuffer(totaLen);
    std::cout<< "total len of recv buff: " << recvBuffer.size() <<std::endl;
    len = recv(fd, recvBuffer.data(), recvBuffer.size(), 0);
    std::cout << "read " << len << " bytes from socket buffer" << std::endl;
    std::cout<< "total len of recv buff: " << recvBuffer.size() <<std::endl;
    ssize_t tmp;
    ssize_t totalRecv = len;
    if(totaLen <= (uint32_t)SSIZE_MAX)
    {
        tmp = (ssize_t)totaLen;
    }
    else
    {
        std::cout<< "too big number on recvMsg " << totaLen <<std::endl;
        return std::nullopt;
    }
    if (len == recvBuffer.size())
    {
        //do nothing now
        std::cout<< "len == tmp" <<std::endl;
    }
    else if (len < tmp)
    {
        tmp = tmp - len;
        while(tmp-len >= 0)
        {
            len = recv(fd, recvBuffer.data()+len, tmp-len, 0);
            std::cout<< "contunie read " << len << " of bytes";
            tmp = tmp - len;
            if(len < 0)
            {
                std::cout<< "contunie read error len: " << len << " actual total recv: " << totalRecv << " of bytes" <<std::endl;
                break;
            }
            else
            {
                totalRecv = totalRecv + len;
                std::cout<< "contunie read len: " << len << " actual total recv: " << totalRecv << " of bytes" <<std::endl;
            }
        }
    }
    else
    {
        std::cout << "recv the wrong len" << std::endl;
        return std::nullopt;
    }
    // 这个部分返回的是vector<char>,转为string使用构造函数std::string str(ret.data(), ret.size())
    std::cout<< "actual total recv len of this thime: " << totalRecv << "--" << recvBuffer.size() <<std::endl;
    return Msg::deserialize(recvBuffer.data(), totalRecv);
}
