## 等待实现
- [ ] id管理，后续考虑jwt，uuid（雪花算法），并且集中管理，防止冲突
## 设计思路
- [ ] 匿名模式
- [ ] 有名模式(登录认证)
### 匿名模式
- [x] 网络通信（QTcpSocket）
## 数据结构设计
- [x] 消息头(VioletProtHeader)
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
### 消息体
- 变长数据，具体内容根据消息类型而定
```
std::vector<char> content;
```

## 功能模块
- [x] 网络通信（QTcpSocket）
- [ ] 数据解析（JSON/Protobuf）