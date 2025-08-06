#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <vector>
#include <arpa/inet.h>

#pragma pack(push, 1)
struct VioletProtHeader {
    uint32_t magic;         // 魔数 0x43484154 ("CHAT")
    uint16_t version;       // 协议版本
    uint16_t type;  // 消息类型
    uint32_t length;   // 消息体长度
    uint32_t timestamp;     // 时间戳(可选)
    uint32_t checksum;      // 头部校验和(可选)
};
struct VioletProtFace
{
    char command[12];     // 请求类型 0=注册 1=登录 2=私聊
    bool unlogin;         // 这个设计是为匿名用户，只有匿名用户，这个值为真
    char username[32];   // 用户名(固定长度)
    char password[64];   // 密码(设计为加密后，初始用明文)
    uint8_t encrypt;    // 加密类型 0=无 1=MD5 2=AES
    uint8_t os;         // 操作系统类型 0=Unknown 1=Windows 2=Linux 3=android...
};
#pragma pack(pop)

/**
 * @brief The MessageTypes enum
 * 暂时不启用
 */
enum MessageTypes {
    MT_TEXT = 0,            // 文本消息
    MT_IMAGE = 1,           // 图片
    MT_FILE_START = 2,      // 文件传输开始
    MT_FILE_CHUNK = 3,      // 文件分块
    MT_FILE_END = 4,        // 文件传输结束
    MT_HEARTBEAT = 5,       // 心跳包
    MT_USER_ONLINE = 6,     // 用户上线通知
    MT_USER_OFFLINE = 7,    // 用户下线通知
    MT_PROTOCOL_UPGRADE = 8 // 协议升级请求
};

class Msg
{
public:
    VioletProtHeader header;
    std::vector<char> content;
    std::vector<char> serialize() const {
        std::vector<char> packet(sizeof(VioletProtHeader) + content.size());
        memcpy(packet.data(), &header, sizeof(header));
        if(!content.empty())
        {
            memcpy(packet.data() + sizeof(header), content.data(), content.size());
        }
        return packet;
    }
    static std::optional<Msg> deserialize(const char *data, size_t length)
    {
        if(length < sizeof(VioletProtHeader))
            return std::nullopt;
        Msg msg;
        memcpy(&msg.header, data, sizeof(msg.header));
        if (ntohl(msg.header.magic) != 0x43484154)
        {
            return std::nullopt;
        }
        size_t excepted_len = sizeof(msg.header) + ntohl(msg.header.length);
        if(length < excepted_len)
        {
            return std::nullopt;
        }
        size_t body_len = ntohl(msg.header.length);
        if(body_len > 0)
        {
            msg.content.assign(data+sizeof(msg.header), data+sizeof(msg.header)+body_len);
        }
        return msg;
    }
};

#endif // PROTOCOL_H
