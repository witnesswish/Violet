## 等待实现
- [ ] id管理，后续考虑jwt，uuid（雪花算法），并且集中管理，防止冲突
## 设计思路
### [ ] 匿名模式
- [ ] 有名模式(登录认证)
### 匿名模式
- [x] 网络通信（QTcpSocket）
## 数据结构设计
- [x] 消息头(VioletProtHeader)
- [X] 消息脖子(VioletProtNeck)
- [x] 消息体(变长)
### 消息头
```
struct VioletProtHeader {
    uint32_t magic;         // 魔数 0x43484154 ("CHAT")
    uint16_t version;       // 协议版本
    uint16_t type;  // 消息类型
    uint32_t length;   // 消息体长度
    uint32_t timestamp;     // 时间戳
    uint32_t checksum;      // 头部校验和
};
```
### 消息脖子
```
struct VioletProtNeck
{
    char command[12];     // 请求类型，规定为大写，匿名群聊[NONG]，匿名私聊[NONP],
    bool unlogin;         // 这个设计是为匿名用户，只有匿名用户，这个值为真
    char username[32];   // 用户名(固定长度)
    char password[64];   // 密码(设计为加密后，初始用明文)
    uint8_t encrypt;    // 加密类型 0=无 1=MD5 2=AES
    uint8_t os;         // 操作系统类型 0=Unknown 1=Windows 2=Linux 3=android...
    uint8_t mto;        // 发送的对象，发送给哪个用户，或者哪个群，使用id
    uint8_t mfrom;      // 数据发来的对象，是哪个用户，这个服务器收到消息的第一时间写入
    //uint8_t tmp;      // 后续有了再继续加
};
```
#### char command[12]
- 说明  
这个字段用来请求和回复，如果这个字段不对，会丢弃消息，规定这个字段使用小写
- 匿名模式   
1. 模式确认，客户端发送[nonreq]请求进行匿名，服务器回复[nonsucc]确认，[moderr]为错误
2. 群聊，客户端发送[nong]进行匿名群聊，服务器回复[nongsucc]确认，[nongerr]为错
2. 入群，客户端发送[nonig]申请加入群聊，需要说明的是初始默认不加入匿名群聊，需要在wild里点击加入
3. 群发，服务器发送[nongb]进行群聊转发，
3. 私聊，客户端发送[nonp]进行匿名私聊，服务器回复[nonpsucc]确认，[nonperr]为错
1. 私发，服务器发送[nonpb]进行私聊转发，
#### char username[32]
- 说明
这个字段代表用户名，当客户端发给服务器时，代表来自用户，当服务器发给客户端时，代表来自哪个用户，客户端要维护好自己的名字
- 特别说明
当匿名模式时，这个用户名会使用临时id代替
### 消息体
- 变长数据，具体内容根据消息类型而定
```
std::vector<char> content;
```

## 功能模块
- [x] 网络通信（QTcpSocket）
- [ ] 数据解析（JSON/Protobuf）

## 软件流程
- 打开软件 
- 请求服务器 -请求失败，重新请求或退出
- 请求成功，选择使用模式 -使用匿名模式
- 发送请求信息，包括选择模式[unlogin]，
- 服务器将连接发送给unlogin处理