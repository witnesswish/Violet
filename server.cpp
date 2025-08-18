#include "server.h"

std::unordered_map<int, UserRecvBuffer> Server::userRecvBuffMap;

Server::Server()
{
    sock = 0;
    epfd = 0;
    serAddr.sin_port = htons(3434);
    serAddr.sin_addr.s_addr = INADDR_ANY;
    redis.connectRedis("127.0.0.1", 6379);
    emailreg = std::regex("(^[a-zA-Z0-9-_]+@[a-zA-Z0-9-_]+(\\.[a-zA-Z0-9-_]+)+)");
    //说明，最多10个中文
    namereg = std::regex("[a-zA-z0-9\u4e00-\u9fa5]{1,30}");
    running = true;
}
Server::~Server()
{
    closeServer();
    redis.disconnectRedis();
}
int Server::getRecvSize(int fd)
{
    int bytes;
    if (ioctl(fd, FIONREAD, &bytes) < 0)
    {
        perror("read socket recv buffer error");
        return -1;
    }
    return bytes;
}

void Server::init()
{
    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror("socket error");
        exit(-1);
    }
    int optval = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval)))
    {
        perror("setsockopt error");
        exit(-1);
    }
    if (bind(sock, (const struct sockaddr *)&serAddr, sizeof(serAddr)) < 0)
    {
        perror("bind error");
        exit(-1);
    }
    if (listen(sock, 10) < 0)
    {
        perror("listen error");
        exit(-1);
    }
    epfd = epoll_create(EPOLL_SIZE);
    if (epfd < 0)
    {
        perror("epfd error");
        exit(-1);
    }
    addfd(sock, epfd);
}

void Server::startServer()
{
    static struct epoll_event events[EPOLL_SIZE];
    init();
    while (running)
    {
        int epoll_events_count = epoll_wait(epfd, events, EPOLL_SIZE, -1);
        if (epoll_events_count < 0)
        {
            perror("epoll event count error");
            break;
        }
        for (int i = 0; i < epoll_events_count; ++i)
        {
            int fd = events[i].data.fd;
            if (fd == sock)
            {
                struct sockaddr_in clientAddr;
                socklen_t clientAddrLength = sizeof(struct sockaddr_in);
                int client = accept(sock, (struct sockaddr *)&clientAddr, &clientAddrLength);
                std::cout << "client connection from: "
                          << inet_ntoa(clientAddr.sin_addr) << ":"
                          << ntohs(clientAddr.sin_port) << ", clientfd = #"
                          << client << std::endl;
                addfd(client, epfd);
                // clients_list.push_back(clientfd);一些操作
                std::string welcome = "welcome, your id is #";
                welcome += std::to_string(client);
                welcome += ", enjoy yourself";
                std::cout << "welcome: " << welcome << std::endl;
                sr.sendMsg(client, 0, welcome);
            }
            else
            {
                int bytesReady = getRecvSize(fd);
                while (bytesReady > 41 || bytesReady == 0)
                {
                    std::cout << "read from client(clientID = #" << fd << ")" << std::endl;
                    std::optional<Msg> ret=Msg{};
                    auto ixt = userRecvBuffMap.find(fd);
                    if(ixt != userRecvBuffMap.end())
                    {
                        UserRecvBuffer &murb = ixt->second;
                        if(murb.fd != fd)
                        {
                            std::cout<< "something error i don't know if this shown, just tag it, location 1" <<std::endl;
                            continue;
                        }
                        else
                        {
                            ret = sr.recvMsg(fd, murb.expectLen - murb.actuaLen);
                            if(ret != std::nullopt)
                            {
                                murb.actuaLen += ret->header.checksum;
                                murb.recvBuffer.insert(murb.recvBuffer.end(), ret->content.begin(), ret->content.end());
                                if(ret->header.checksum == 0)
                                {
                                    std::cout<< "read special byte, get 0, knicking user out" <<std::endl;
                                    unlogin.removeUnlogin(fd);
                                    vofflineHandle(fd);
                                    bytesReady = 1;
                                }
                                if(murb.actuaLen < murb.expectLen)
                                {
                                    continue;
                                    bytesReady = 1;
                                }
                                else
                                {
                                    //ret = Msg::deserialize(murb.recvBuffer.data(), murb.expectLen+sizeof(ret->header)+sizeof(ret->neck));
                                    ret = Msg::deserialize(murb.recvBuffer.data(), murb.recvBuffer.size());
                                    userRecvBuffMap.erase(fd);
                                    bytesReady = 1;
                                }
                            }
                            else
                            {
                                std::cout<< "read special byte error, contunie" <<std::endl;
                                bytesReady = 1;
                                continue;
                            }
                        }
                    }
                    else
                    {
                        ret = sr.recvMsg(fd, -1);
                        std::cout<< "content length: " << ret->header.length << "--" << ret->header.checksum << "--" << ntohl(ret->header.length) <<std::endl;
                    }
                    if((ssize_t)ret->header.checksum < ntohl(ret->header.length))
                    {
                        if(!ret.has_value())
                        {
                            std::cout<< "ret empty" <<std::endl;
                        }
                        else
                            std::cout<< "ret not empty" <<std::endl;
                        UserRecvBuffer urbf;
                        urbf.fd = fd;
                        urbf.expectLen = ntohl(ret->header.length);
                        urbf.actuaLen = ret->header.checksum;
                        urbf.recvBuffer = ret.value().serialize();
                        //memcpy(urbf.recvBuffer.data(), &(ret->header), sizeof(ret->header));
                        //memcpy(urbf.recvBuffer.data()+sizeof(ret->header), &(ret->neck), sizeof(ret->neck));
                        //urbf.recvBuffer.insert(urbf.recvBuffer.end(), ret->content.begin(), ret->content.end());
                        userRecvBuffMap[fd] = urbf;
                        std::cout<< "not recving all data, stash to violet recv cache, continue, total: " << ntohl(ret->header.length)
                                 <<" recv: " << ret->header.checksum <<std::endl;
                        bytesReady = 1;
                        continue;
                    }
                    if (ret == std::nullopt)
                    {
                        std::cout << "sr return null opt" << std::endl;
                    }
                    else
                    {
                        if (ntohl(ret->header.length) == 1)
                        {
                            bytesReady = 1;
                        }
                        // 这里取值判断是因为要对fd进行一系列处理，这样虽然有点麻烦，但是后面看看机会再修改一下
                        // 前面已经close过一次了，所以直接处理剩余的步骤，从list移除等行为
                        else if (ret->header.length == 0)
                        {
                            //HMSET %s fd %s id %s stat %s
                            unlogin.removeUnlogin(fd);
                            vofflineHandle(fd);
                            bytesReady = 1;
                        }
                        else
                        {
                            bytesReady = getRecvSize(fd);
                            ret->neck.mfrom = fd;
                            if (!ret->neck.unlogin)
                            {
                                std::string command(ret->neck.command);
                                std::string username(ret->neck.name);
                                std::string password(ret->neck.pass);
                                std::string ccdemail(ret->neck.email);
                                std::string content;
                                content.reserve(ret->content.size()+10);  // 预分配内存
                                content.assign(ret->content.begin(), ret->content.end());
                                std::cout << command << "-" << username << "-" << password << "-" << content << std::endl;
                                if (command == "vreg")
                                {
                                    vregister(fd, username, password, ccdemail);
                                }
                                if (command == "vlogin")
                                {
                                    vlogin(fd, username, password);
                                }
                                if (command == "vaddf")
                                {
                                    vaddFriend(fd, username, content);
                                }
                                if (command == "vaddg")
                                {
                                    vaddGroup(fd, username, content);
                                }
                                if (command == "vcrtg")
                                {
                                    vcreateGroup(fd, username, content);
                                }
                                if(command == "vpc")
                                {
                                    vprivateChat(fd, username, password, content);
                                }
                                if(command == "vgc")
                                {
                                    vgroupChat(fd, username, password, content);
                                }
                            }
                            if (ret->neck.unlogin)
                            {
                                std::string text = std::string(ret->content.begin(), ret->content.end());
                                std::string command(ret->neck.command);
                                std::cout << "command: " << command << std::endl;
                                if (command == std::string("nonreq"))
                                {
                                    VioletProtNeck neck = {};
                                    strcpy(neck.command, "nonsucc");
                                    strcpy(neck.name, std::to_string(fd).c_str());
                                    std::string tmp = std::string("violet");
                                    sr.sendMsg(fd, neck, tmp);
                                }
                                if (command == std::string("nong"))
                                {
                                    std::cout << "nong content to string: " << std::string(ret->content.begin(), ret->content.end()) << std::endl;
                                    unlogin.sendBordcast(fd, std::string(ret->content.begin(), ret->content.end()));
                                }
                                if (command == std::string("nonig"))
                                {
                                    unlogin.addNewUnlogin(fd);
                                }
                                if (command == std::string("nonqg"))
                                {
                                    unlogin.removeUnlogin(fd);
                                }
                                if (command == std::string("nonp"))
                                {
                                    unlogin.privateChate(fd, ret->neck.mto, text);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    closeServer();
}
void Server::closeServer()
{
    running = false;
    if (epfd)
        close(epfd);
    if (sock)
        close(sock);
}

void Server::vregister(int fd, std::string username, std::string password, std::string email)
{
    if(!regex_match(email, emailreg))
    {
        VioletProtNeck neck = {};
        strcpy(neck.command, (const char *)"vregerr");
        std::string tmp("email type error");
        sr.sendMsg(fd, neck, tmp);
        return;
    }
    if(!regex_match(username, namereg))
    {
        VioletProtNeck neck = {};
        strcpy(neck.command, (const char *)"vregerr");
        std::string tmp("name type error");
        sr.sendMsg(fd, neck, tmp);
        return;
    }
    int ret = loginCenter.vregister(username, password, email, "salt");
    if (ret < 0)
    {
        VioletProtNeck neck = {};
        strcpy(neck.command, (const char *)"vregerr");
        std::string tmp("violet");
        sr.sendMsg(fd, neck, tmp);
        return;
    }
    if(ret == 1)
    {
        VioletProtNeck neck = {};
        strcpy(neck.command, (const char *)"vregerr");
        std::string tmp("username exists");
        sr.sendMsg(fd, neck, tmp);
        return;
    }
    if(ret == 2)
    {
        VioletProtNeck neck = {};
        strcpy(neck.command, (const char *)"vregerr");
        std::string tmp("email exists");
        sr.sendMsg(fd, neck, tmp);
        return;
    }
    if(ret == 0)
    {
        VioletProtNeck neck = {};
        strcpy(neck.command, (const char *)"vregsucc");
        std::string tmp("violet");
        sr.sendMsg(fd, neck, tmp);
    }
    VioletProtNeck neck = {};
    strcpy(neck.command, (const char *)"vregerr");
    std::string tmp("violet");
    sr.sendMsg(fd, neck, tmp);
}

void Server::vaddFriend(int fd, std::string reqName, std::string friName)
{
    if(!regex_match(friName, namereg))
    {
        VioletProtNeck neck = {};
        strcpy(neck.command, (const char *)"vaddferr");
        std::string tmp("type error");
        sr.sendMsg(fd, neck, tmp);
        return;
    }
    if(!regex_match(reqName, namereg))
    {
        VioletProtNeck neck = {};
        strcpy(neck.command, (const char *)"vaddferr");
        std::string tmp("type error");
        sr.sendMsg(fd, neck, tmp);
        return;
    }
    VioletProtNeck neck = {};
    int ret = loginCenter.vaddFriend(reqName, friName);
    if (ret == 0)
    {
        strcpy(neck.command, "vaddfsucc");
        memcpy(neck.name, friName.c_str(), sizeof(neck.name));
        sr.sendMsg(fd, neck, friName);
        return;
    }
    if(ret == 1)
    {
        std::cout<< "add friend error, function return: " << ret <<std::endl;
        strcpy(neck.command, "vaddferr");
        memcpy(neck.name, friName.c_str(), sizeof(neck.name));
        std::string tmp = std::string("friend you add is not exists: ") + friName;
        sr.sendMsg(fd, neck, tmp);
        return;
    }
    std::cout<< "add friend error, function return: " << ret <<std::endl;
    strcpy(neck.command, "vaddferr");
    std::string tmp("violet");
    sr.sendMsg(fd, neck, tmp);
}

void Server::vaddGroup(int fd, std::string reqName, std::string groupName)
{
    if(!regex_match(groupName, namereg))
    {
        VioletProtNeck neck = {};
        strcpy(neck.command, (const char *)"vaddgerr");
        std::string tmp("type error");
        sr.sendMsg(fd, neck, tmp);
        return;
    }
    if(!regex_match(reqName, namereg))
    {
        VioletProtNeck neck = {};
        strcpy(neck.command, (const char *)"vaddgerr");
        std::string tmp("type error");
        sr.sendMsg(fd, neck, tmp);
        return;
    }
    VioletProtNeck neck = {};
    int ret = loginCenter.vaddGroup(reqName, groupName, fd);
    std::cout << "add g ret: " << ret << std::endl;
    if (ret == 0)
    {
        strcpy(neck.command, "vaddgsucc");
        memcpy(neck.name, groupName.c_str(), sizeof(neck.name));
        sr.sendMsg(fd, neck, groupName);
        return;
    }
    if(ret == 1)
    {
        strcpy(neck.command, "vaddgerr");
        memcpy(neck.name, groupName.c_str(), sizeof(neck.name));
        std::string tmp("group not exists");
        sr.sendMsg(fd, neck, tmp);
        return;
    }
    strcpy(neck.command, "vaddgerr");
    std::string tmp("violet");
    sr.sendMsg(fd, neck, tmp);
}

void Server::vcreateGroup(int fd, std::string reqName, std::string groupName)
{
    if(!regex_match(groupName, namereg))
    {
        VioletProtNeck neck = {};
        strcpy(neck.command, "vcrtgerr");
        std::string tmp("type error");
        sr.sendMsg(fd, neck, tmp);
        return;
    }
    if(!regex_match(reqName, namereg))
    {
        VioletProtNeck neck = {};
        strcpy(neck.command, "vcrtgerr");
        std::string tmp("type error");
        sr.sendMsg(fd, neck, tmp);
        return;
    }
    VioletProtNeck neck = {};
    int ret = loginCenter.vcreateGroup(reqName, groupName, fd);
    if (ret < 0)
    {
        strcpy(neck.command, "vcrtgerr");
        std::string tmp("violet");
        sr.sendMsg(fd, neck, tmp);
        return;
    }
    if (ret == 0)
    {
        strcpy(neck.command, "vcrtgsucc");
        sr.sendMsg(fd, neck, groupName);
        return;
    }
    if (ret == 1)
    {
        strcpy(neck.command, "vcrtgerr");
        std::string tmp("exists");
        sr.sendMsg(fd, neck, tmp);
        return;
    }
}

void Server::vprivateChat(int fd, std::string reqName, std::string friName, std::string content)
{
    std::cout<< "vpc params: " << reqName << "-" << friName << "-" << content <<std::endl;
    VioletProtNeck neck = {};
    int friId = loginCenter.vprivateChat(friName);
    std::cout<< "login return fid: " << friId <<std::endl;
    if(friId < 0)
    {
        strcpy(neck.command, "vpcerr");
        std::string tmp("user not online, message will be sent after user online");
        sr.sendMsg(fd, neck, tmp);

        // 下面是做缓存，缓冲用redis的sorted set来做，zadd 用户名 时间戳 拼接后的消息
        // 拼接消息的格式：发送者|消息内容
        // 发送的时候按次取出消息，做解拼接处理，一次取20条，取完之后，zcard key做判断还有没有数据，循环
        auto c = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(c);
        std::string tmptime = std::to_string(t);
        const std::string tmpstr = reqName + "|" + content;
        std::string tmpname = friName + "msgcache";
        auto ret = redis.execute("ZADD %s %s %s", tmpname.c_str(), tmptime.c_str(), tmpstr.c_str());
        if(ret == std::nullopt)
        {
            std::cout<< "redis execute error on prichat offline" <<std::endl;
        }
        return;
    }
    strcpy(neck.command, "vpcb");
    memcpy(neck.name, reqName.c_str(), sizeof(reqName));
    sr.sendMsg(friId, neck, content);
}

void Server::vgroupChat(int fd, std::string reqName, std::string gname, std::string content)
{
    loginCenter.vgroupChat(fd, reqName, gname, content);
}

void Server::vofflineHandle(int fd)
{
    loginCenter.vofflineHandle(fd);
}

void Server::vlogin(int fd, std::string username, std::string password)
{
    //只需要拦截用户名，密码我的后续计划是在前端进行一次加密，再传输，先不写规则
    if(!regex_match(username, namereg))
    {
        VioletProtNeck neck = {};
        strcpy(neck.command, "vloginerr");
        std::string tmp("type error");
        sr.sendMsg(fd, neck, tmp);
        return;
    }
    memset(&u, 0, sizeof(u));
    std::string userinfo;
    int ret = loginCenter.vlogin(fd, username, password, userinfo);
    if (ret < 0)
    {
        std::cout<< "login failure, function return: " << ret <<std::endl;
        VioletProtNeck neck = {};
        strcpy(neck.command, (const char *)"vloginerr");
        std::string tmp("violet");
        sr.sendMsg(fd, neck, tmp);
        return;
    }
    if(ret == 1)
    {
        std::cout<< "login failure, function return: " << ret <<std::endl;
        VioletProtNeck neck = {};
        strcpy(neck.command, (const char *)"vloginerr");
        std::string tmp("incorrect");
        sr.sendMsg(fd, neck, tmp);
        return;
    }
    if (ret == 2)
    {
        std::cout<< "login failure, function return: " << ret <<std::endl;
        VioletProtNeck neck = {};
        strcpy(neck.command, (const char *)"vloginerr");
        std::string tmp("incorrect");
        sr.sendMsg(fd, neck, tmp);
        return;
    }
    if(ret == 3)
    {
        std::cout<< "login failure, function return: " << ret <<std::endl;
        VioletProtNeck neck = {};
        strcpy(neck.command, (const char *)"vloginerr");
        std::string tmp("incorrect");
        sr.sendMsg(fd, neck, tmp);
        return;
    }
    // for (const auto &it : u.friends)
    // {
    //     std::cout << it << std::endl;
    // }
    // for (const auto &it : u.groups)
    // {
    //     std::cout << it << std::endl;
    // }
    VioletProtNeck neck = {};
    strcpy(neck.command, (const char *)"vloginsucc");
    memcpy(neck.name, username.c_str(), sizeof(neck.name));
    sr.sendMsg(fd, neck, userinfo);
    std::cout<< "login success, user: " << username <<std::endl;

    //get cache and send
    std::string tmpnum;
    std::string tmpname = username + "msgcache";
    auto ret1 = redis.execute("ZCARD %s", tmpname.c_str());
    if(ret1 == std::nullopt)
    {
        std::cout<< "redis execute error on login get offline msg" <<std::endl;
    }
    else
    {
        //optional<vector<string>>
        for(auto &it : ret1.value())
        {
            std::cout<< "read num of msgcache on login: " << it <<std::endl;
            tmpnum = it;
        }
    }
    auto tmpx = std::stoi(tmpnum);
    std::cout<< "tmpx: " << tmpx <<std::endl;
    while(tmpx > 0)
    {
        if(tmpx-20 >= 0)
        {
            //auto ret2 = redis.execute("ZRANG %s 0 19 WITHSCORES", username.c_str()); //if you need timestamp, with option withscores
            auto ret2 = redis.execute("ZRANGE %s 0 19", tmpname.c_str());
            if(ret2 == std::nullopt)
            {
                std::cout<< "redis execute error on login get offline msg" <<std::endl;
            }
            else
            {
                //optional<vector<string>>
                for(auto &it : ret2.value())
                {
                    auto pos = it.find('|');
                    if(pos > 0 && pos < it.size())
                    {
                        std::string from = it.substr(0, pos);
                        std::string content = it.substr(pos + 1);
                        VioletProtNeck neck = {};
                        strcpy(neck.command, "vpcache");
                        memcpy(neck.name, from.c_str(), sizeof(neck.name));
                        //std::cout << "ser recv: " << userinfo << std::endl;
                        sr.sendMsg(fd, neck, content);
                    }
                }
            }
            auto ret = redis.execute("ZREMRANGEBYRANK %s 0 19", tmpname.c_str());
            if(ret == std::nullopt)
            {
                std::cout<< "redis execute error on login at del 20" <<std::endl;
            }
            tmpx = tmpx - 20;
        }
        else if(tmpx-10 >= 0)
        {
            auto ret2 = redis.execute("ZRANGE %s 0 9", tmpname.c_str());
            if(ret2 == std::nullopt)
            {
                std::cout<< "redis execute error on login get offline msg" <<std::endl;
            }
            else
            {
                //optional<vector<string>>
                for(auto &it : ret2.value())
                {
                    auto pos = it.find('|');
                    if(pos > 0 && pos < it.size())
                    {
                        std::string from = it.substr(0, pos);
                        std::string content = it.substr(pos+1);
                        VioletProtNeck neck = {};
                        strcpy(neck.command, "vpcache");
                        memcpy(neck.name, from.c_str(), sizeof(neck.name));
                        //std::cout << "ser recv: " << userinfo << std::endl;
                        sr.sendMsg(fd, neck, content);
                    }
                }
            }
            auto ret = redis.execute("ZREMRANGEBYRANK %s 0 9", tmpname.c_str());
            if(ret == std::nullopt)
            {
                std::cout<< "redis execute error on login  at del 10" <<std::endl;
            }
            tmpx = tmpx - 10;
        }
        else if(tmpx-5 >= 0)
        {
            auto ret2 = redis.execute("ZRANGE %s 0 4", tmpname.c_str());
            if(ret2 == std::nullopt)
            {
                std::cout<< "redis execute error on login get offline msg" <<std::endl;
            }
            else
            {
                //optional<vector<string>>
                for(auto &it : ret2.value())
                {
                    auto pos = it.find('|');
                    if(pos > 0 && pos < it.size())
                    {
                        std::string from = it.substr(0, pos);
                        std::string content = it.substr(pos+1);
                        VioletProtNeck neck = {};
                        strcpy(neck.command, "vpcache");
                        memcpy(neck.name, from.c_str(), sizeof(neck.name));
                        //std::cout << "ser recv: " << userinfo << std::endl;
                        sr.sendMsg(fd, neck, content);
                    }
                }
            }
            auto ret = redis.execute("ZREMRANGEBYRANK %s 0 4", tmpname.c_str());
            if(ret == std::nullopt)
            {
                std::cout<< "redis execute error on login  at del 5" <<std::endl;
            }
            tmpx = tmpx - 5;
        }
        else
        {
            auto ret2 = redis.execute("ZRANGE %s 0 0", tmpname.c_str());
            if(ret2 == std::nullopt)
            {
                std::cout<< "redis execute error on login get offline msg" <<std::endl;
            }
            else
            {
                //optional<vector<string>>
                for(auto &it : ret2.value())
                {
                    auto pos = it.find('|');
                    if(pos > 0 && pos < it.size())
                    {
                        std::string from = it.substr(0, pos);
                        std::string content = it.substr(pos+1);
                        VioletProtNeck neck = {};
                        strcpy(neck.command, "vpcache");
                        memcpy(neck.name, from.c_str(), sizeof(neck.name));
                        //std::cout << "ser recv: " << userinfo << std::endl;
                        sr.sendMsg(fd, neck, content);
                    }
                }
            }
            auto ret = redis.execute("ZREMRANGEBYRANK %s 0 0", tmpname.c_str());
            if(ret == std::nullopt)
            {
                std::cout<< "redis execute error on login  at del 1" <<std::endl;
            }
            tmpx = tmpx - 1;
        }
    }
    auto ret2 = redis.execute("DEL %s", tmpname.c_str());
    if(ret2 == std::nullopt)
    {
        std::cout<< "redis execute error on login at endline" <<std::endl;
    }
}
