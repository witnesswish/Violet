#include "protocol.h"

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

std::optional<Msg> SRHelper::recvMsg(int fd)
{
    VioletProtHeader header;
    VioletProtNeck neck;
    ssize_t len = recv(fd, &header, sizeof(header), MSG_PEEK);
    std::cout << "peek len: " << len << " of recv" << std::endl;
    if (len == 0)
    {
        close(fd);
        std::cout << "client #" << fd << " closed" << std::endl;
        Msg msg = {};
        // 这里我的想法是找一个字段，用来存状态，但是先用这个吧
        msg.header.length = 0;
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
            return msg;
        }
        else if (errno == 11)
        {
            Msg msg = {};
            std::cout << "read on nonblock" << std::endl;
            msg.header.length = 1;
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
    uint32_t totaLen = contentLen + sizeof(header) + sizeof(neck);
    std::cout << "total len to malloc " << totaLen << " of recv" << std::endl;
    std::vector<char> recvBuffer(totaLen);
    len = recv(fd, recvBuffer.data(), recvBuffer.size(), 0);
    std::cout << "read " << len << " bytes from socket buffer" << std::endl;
    if (len == static_cast<ssize_t>(recvBuffer.size()))
    {
        //do nothing now
    }
    else if (len < static_cast<ssize_t>(recvBuffer.size()))
    {
        ssize_t tmp = len;
        while(totaLen-tmp > 0)
        {
            len = recv(fd, recvBuffer.data()+len, recvBuffer.size()-len, 0);
            std::cout<< "contunie read " << len << " of bytes";
            tmp += len;
        }
    }
    else
    {
        std::cout << "recv the wrong len" << std::endl;
        return std::nullopt;
    }
    // 这个部分返回的是vector<char>,转为string使用构造函数std::string str(ret.data(), ret.size())
    return Msg::deserialize(recvBuffer.data(), recvBuffer.size());
}

Msg::Msg()
{
    content.reserve(8000);
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

std::optional<Msg> Msg::deserialize(const char *data, size_t length)
{
    if (length < sizeof(VioletProtHeader))
        return std::nullopt;
    Msg msg;
    memcpy(&msg.header, data, sizeof(msg.header));
    memcpy(&msg.neck, data + sizeof(msg.header), sizeof(msg.neck));
    if (ntohl(msg.header.magic) != 0x43484154)
    {
        return std::nullopt;
    }
    size_t excepted_len = sizeof(msg.header) + sizeof(msg.neck) + ntohl(msg.header.length);
    if (length < excepted_len)
    {
        return std::nullopt;
    }
    size_t body_len = ntohl(msg.header.length);
    if (body_len > 0)
    {
        msg.content.assign(data + sizeof(msg.header) + sizeof(msg.neck), data + sizeof(msg.header) + sizeof(msg.neck) + body_len);
    }
    return msg;
}
