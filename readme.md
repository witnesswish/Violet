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
    char command[12];  // 请求类型，规定为小写，匿名群聊[NONG]，匿名私聊[NONP]具体请查阅readme
    bool unlogin;      // 这个设计是为匿名用户，只有匿名用户，这个值为真
    char name[32]; // 用户名(固定长度)
    char email[32];
    char evt[5];
    char pass[64]; // 密码(设计为加密后，初始用明文)
    uint8_t encrypt;   // 加密类型 0=无 1=MD5 2=AES
    uint8_t os;        // 操作系统类型 0=Unknown 1=Windows 2=Linux 3=android...
    uint8_t mto;       // 发送的对象，发送给哪个用户，或者哪个群，使用id
    uint8_t mfrom;     // 数据发来的对象，是哪个用户，这个服务器收到消息的第一时间写入
    // uint8_t tmp;      // 后续有了再继续加
};
```
# 所有的交流协议都在这里定义，如果忘记了，请来这里，记住，以文档为主，要是实现改了，记得改文档，我没信心能认全当时自己的想法
#### char command[12]
- 说明  这个字段用来请求和回复，如果这个字段不对，会丢弃消息，规定这个字段使用小写，一般是在客户端请求后面加`succ`或者`err`，字段只要12，所以请求要短
- 匿名模式   
1. 模式确认，客户端发送`nonreq`请求进行匿名，服务器回复`nonsucc`确认，`moderr`为错误
2. 群聊，客户端发送`nong`进行匿名群聊，服务器回复`nongsucc`确认，`nongerr`为错
2. 入群，客户端发送`nonig`申请加入群聊，需要说明的是初始默认不加入匿名群聊，需要在wild里点击加入，服务器回复`nonigsucc`确认，`nonigerr`为错
3. 群发，服务器发送`nongb`进行群聊转发，
3. 私聊，客户端发送`nonp`进行匿名私聊，使用`mto`带上对方id，服务器回复`nonpsucc`确认，`nonperr`为错
1. 私发，服务器发送`nonpb`进行私聊转发，
5. 退出，客户端发送`nonqg`表示退出群聊，服务器回复`nonqgsucc`确认
- 登录模式  

        说明一下，群组可以直接创建，也就是说，可以有一个人的群组
1. 注册，客户端发送`vreg`加信息申请注册，服务器回复`vregsucc`表示成功，`vregerr`表示失败，
2. 登录，客户端发送`vlogin`加信息，服务器回复`vloginsucc`表示成功，同时带上群组和好友信息，`vloginerr`代表失败
3. 加好友，客户端发送`vaddf`加信息，服务器回复`vaddfsucc`
4. 加群组，客户端发送`vaddg`加信息，
5. 创建群组，客户端发送`vcrtg`加信息，
6. 上线广播，服务器发送`vbul`加信息,广播里 neck.name 字段带用户名
6. 下线广播，服务器发送`vbol`，neck.name带用户名
6. 群聊请求，客户端发送`vgc`,这种情况，使用 pass 来的带上群组名称，如果失败返回`vgcerr`，成功不返回
6. 群聊广播，服务器发送`vgcb`,使用 neck.name 字段带上群组名字，neck.pass 带上请求方用户名
7. 私聊请求，客户端发送`vpc`,这种情况，使用 pass 来带对方用户名，如果失败返回`vpcerr`，成功不返回
8. 私聊转发，服务器发送`vpcb`,使用 neck.name 字段带上请求对象用户名
9. 缓存转发，如果用户登录的时候，有缓存，那么客户端发送`vpcache`带上信息，使用 neck.name 表示发送者

        特别说明：服务器做缓存用redis，将其它消息从尾部

#### char name[32]
- 说明
这个字段代表用户名，当客户端发给服务器时，代表来自用户，当服务器发给客户端时，代表来自哪个用户，客户端要维护好自己的名字
- 特别说明
当匿名模式时，这个用户名
#### uint32_t length
- 说明
约定这个正常发送这个字段不为0，也就是说，如果是单纯发送命令和服务器交互，也必须带上内容，一般拼接`violet`
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


## 数据库
- 用户表
```
CREATE TABLE user (
    uid INT PRIMARY KEY AUTO_INCREMENT,
    username VARCHAR(30) UNIQUE NOT NULL,
    nickname VARCHAR(30),
    email VARCHAR(50) UNIQUE NOT NULL,
    password VARCHAR(255) NOT NULL,
    salt VARCHAR(170) NOT NULL,
    avat VARCHAR(255),
    stat ENUM('inactive', 'banned', 'normal') DEFAULT 'normal',
    last_login TIMESTAMP,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
);violet
```
- 好友表
```
CREATE TABLE user_friend (
    ufid INT PRIMARY KEY AUTO_INCREMENT,
    uid1 INT NOT NULL,
    uid2 INT NOT NULL,
    stat ENUM('pending', 'accepted', 'blocked', 'bad') DEFAULT 'pending',
    reqid INT NOT NULL COMMENT '谁发起的请求或操作',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    FOREIGN KEY (uid1) REFERENCES user(uid),
    FOREIGN KEY (uid2) REFERENCES user(uid),
    FOREIGN KEY (reqid) REFERENCES user(uid)
);
```
- 群组
```
CREATE TABLE user_group (
    gid INT PRIMARY KEY AUTO_INCREMENT,
    gname VARCHAR(60) NOT NULL,
    g_description TEXT,
    gowner INT NOT NULL,
    gavat VARCHAR(255),
    gstat ENUM('normal', 'banned') DEFAULT 'normal',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    FOREIGN KEY (gowner) REFERENCES user(uid)
);
```
- 群组成员
```
CREATE TABLE if not exists group_member (
    gmid INT PRIMARY KEY AUTO_INCREMENT,
    gid INT NOT NULL,
    uid INT NOT NULL,
    grole ENUM('owner', 'admin', 'member') DEFAULT 'member',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (gid) REFERENCES user_group(gid),
    FOREIGN KEY (uid) REFERENCES user(uid),
    UNIQUE KEY (gid, uid)
);
```
- 
