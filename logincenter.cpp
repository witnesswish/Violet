#include <iostream>
#include <sstream>
#include <cstdint>

#include "mariadbhelper.h"
#include "logincenter.h"

/**
 * @brief LoginCenter::LoginCenter
 * 1. 把所有群组都加载进来存到redis，用set存，查询的时候会返回redis-reply-array
 * 2. 在用户登录的时候，把用户信息存到redis，用hash存 hmset 登陆者 username name fd 7 id 1，私聊的时候hget 登陆者 fd获取fd转发，如果没有找到，再想想怎么存消息，计划用redis list
 * 3. 自己维护一条list<{groupname, fd}> onlinegGMember,当用户群发消息的时候直接读条群发
 */
std::map<std::string, std::set<int>> LoginCenter::onlineGUMap{};
std::map<std::string, std::list<int>> LoginCenter::onlineUserFriend{};
std::map<int, std::string> LoginCenter::onlineUser{};
LoginCenter::LoginCenter()
{
    redis.connectRedis("127.0.0.1", 6379);
    std::vector<sql::SQLString> params = {"1"};
    // do not forget to release conn
    MariadbHelper::getInstance().init("violet", "123456", "127.0.0.1", 3306, "violet", 7);
    auto conn = MariadbHelper::getInstance().getConnection();
    auto ret = MariadbHelper::getInstance().query(conn, "SELECT DISTINCT gid, gname FROM user_group WHERE 1=?;", params);
    for (const auto &row : ret)
    {
        std::vector<sql::SQLString> perparams = {row.at("gid").c_str()};
        auto per = MariadbHelper::getInstance().query(conn, "SELECT DISTINCT u.username FROM group_member gm JOIN user u WHERE u.uid=gm.uid AND gid=?;", perparams);
        for(const auto &perrow : per)
        {
            redis.execute("SADD %s %s", row.at("gname").c_str(), perrow.at("username").c_str());
        }
    }
    MariadbHelper::getInstance().releaseConnection(conn);
}

LoginCenter::~LoginCenter()
{
    redis.disconnectRedis();
}

/**
 * @brief LoginCenter::vregister
 * @param username
 * @param password
 * @return 0 for normal, 1 for user exists, 2 for email exists
 */
int LoginCenter::vregister(std::string username, std::string password, std::string email, std::string salt)
{
    std::vector<sql::SQLString> params = {username.c_str()};
    auto conn = MariadbHelper::getInstance().getConnection();
    auto ret = MariadbHelper::getInstance().query((conn), "select username, email from user where username=?;", params);
    if (ret.size() > 1)
    {
        MariadbHelper::getInstance().releaseConnection((conn));
        return 1;
    }
    if(ret.size() == 1)
    {
        for(const auto &row : ret)
        {
            if(email == std::string(row.at("email").c_str()))
            {
                MariadbHelper::getInstance().releaseConnection((conn));
                return 2;
            }
        }
        MariadbHelper::getInstance().releaseConnection((conn));
        return 1;
    }
    if(ret.size() == 0)
    {
        std::vector<sql::SQLString> iparams = {username, username, email, password, salt};
        MariadbHelper::getInstance().execute((conn), "insert into user (username, nickname, email, password, salt) values (?, ?, ?, ?, ?);", iparams);
        MariadbHelper::getInstance().releaseConnection((conn));
        return 0;
    }
    MariadbHelper::getInstance().releaseConnection((conn));
    return -1;
}

/**
 * @brief LoginCenter::login
 * @param username
 * @param password
 * @return -1 for database error
 * 0 for succ, 1 for username not exists, 2 for password error
 */
int LoginCenter::vlogin(int fd, std::string username, std::string password, std::string &userinfo)
{
    auto conn = MariadbHelper::getInstance().getConnection();
    User u;
    std::vector<sql::SQLString> params = {username};
    auto ret = MariadbHelper::getInstance().query((conn), "select uid, username, nickname, password, salt, avat from user where username=?;", params);
    // 如果查出来的数据不是一条或者0条，那就是数据库大概率出问题了
    if (ret.size() != 1)
    {
        MariadbHelper::getInstance().releaseConnection((conn));
        return 1;
    }
    for (const auto &row : ret)
    {
        // vector<map<string, sql::SQLString>> query
        if (password != std::string(row.at("password").c_str()))
        {
            MariadbHelper::getInstance().releaseConnection(std::move(conn));
            return -2;
        }
        auto ret = redis.execute("HMSET %s fd %s id %s stat %s", row.at("username").c_str(), std::to_string(fd).c_str(), row.at("uid").c_str(), (const char *)"normal");
        if(ret == std::nullopt)
        {
            std::cout<< "redis execute error on login" <<std::endl;
        }
    }
    // 到此为止，登录逻辑已经完成，下面是把用户的群组好友等信息返回
    std::vector<sql::SQLString> fparams = {username, username};
    auto friInfo = MariadbHelper::getInstance().query(std::move(conn), "SELECT DISTINCT u.username AS friend_name FROM user_friend f JOIN user u ON f.uid2=u.uid OR f.uid1=u.uid "
                                 "WHERE f.uid1 IN (SELECT uid FROM user WHERE username=?) OR f.uid2 IN ( SELECT uid FROM user WHERE username=?);",
                                 fparams);
    for (const auto &irow : friInfo)
    {
        if (username != std::string(irow.at("username").c_str()))
        {
            //返回好友列表的同时，对所有在线好友广播上线，同时告诉自己有哪些好友是在线的
            u.friends.push_back(std::string(irow.at("username").c_str()));
            auto ret= redis.execute("HGET %s fd", irow.at("username").c_str());
            if(ret != std::nullopt)
            {
                for(auto it : ret.value())
                {
                    VioletProtNeck neck = {};
                    strcpy(neck.command, "vbul");
                    strcpy(neck.name, username.c_str());
                    std::string tmp("violet");
                    sr.sendMsg(std::stoi(it), neck, tmp);
                    onlineUserFriend.clear();
                    std::list<int> tmpList;
                    tmpList.push_back(std::stoi(it));
                    onlineUserFriend[username] = tmpList;
                    std::cout<< "boradcast login get fd from redis: " << std::stoi(it) << "--" << it <<std::endl;
                }
            }
            //std::cout << irow.at("username") << std::endl;
        }
    }
    auto groupInfo = MariadbHelper::getInstance().query(std::move(conn), "SELECT DISTINCT g.gname AS group_name FROM user_group g JOIN group_member gm "
                "WHERE gm.gid=g.gid AND gm.uid IN (SELECT uid FROM user WHERE username=?);",
                params);
    for (const auto &row : groupInfo)
    {
        u.groups.push_back(std::string(row.at("gname").c_str()));
        //维护群组在线用户列表
        updateOnlineGUMap(std::string(row.at("gname").c_str()), fd);
        //std::cout<< "debug login gum: " << onlineGUMap.size() <<std::endl;
        std::string tmpgn = username + "grp";
        auto ret = redis.execute("LPUSH %s %s", tmpgn.c_str(), std::string(row.at("gname").c_str()));
        if(ret == std::nullopt)
        {
            std::cout<< "redis execute error on vofflineHandle" <<std::endl;
        }
    }
    MariadbHelper::getInstance().releaseConnection(std::move(conn));
    userinfo = serializeTwoVector(u.friends, u.groups);
    // update online user
    onlineUser[fd] = username;
    return 0;
}

/**
 * @brief LoginCenter::vaddFriend
 * @param requestName
 * @param friName
 * @return 0 for normal, 1 for fri not exists
 * 理论上来说插入好友关系的时候，应该固定好小的id在前还是大的id在前，不过没关系啦，我写的sql够复杂
 * 我写这段话只是希望下次我看到的时候把这里改一下，这样应该好查一点
 */
int LoginCenter::vaddFriend(std::string requestName, std::string friName)
{
    auto conn = MariadbHelper::getInstance().getConnection();
    std::vector<sql::SQLString> params = {requestName, requestName, friName, friName};
    auto ret = MariadbHelper::getInstance().query(std::move(conn), "SELECT * FROM user_friend uf WHERE ((uf.uid1 IN (SELECT uid FROM user WHERE username=?)) "
                             "OR (uf.uid1 IN (SELECT uid FROM user WHERE username=?))) AND ((uf.uid1 IN (SELECT uid FROM user "
                             "WHERE username=?)) OR (uf.uid1 IN (SELECT uid FROM user WHERE username=?)))",
                             params);
    std::cout<< "add f: " << ret.size() <<std::endl;
    if (ret.size() == 1)
    {
        MariadbHelper::getInstance().releaseConnection(std::move(conn));
        return 0;
    }
    else if (ret.size() == 0)
    {
        std::vector<sql::SQLString> friparams = {friName};
        auto retfri = MariadbHelper::getInstance().query(std::move(conn), "SELECT uid FROM user WHERE username=?;",friparams);
        std::string friuid;
        if(retfri.size() != 1)
        {
            MariadbHelper::getInstance().releaseConnection(std::move(conn));
            return 1;
        }
        else
        {
            // for next version, delete if next version is come
            for(const auto &it : retfri)
            {
                friuid = std::string(it.at("uid").c_str());
            }
        }
        std::vector<sql::SQLString> iparams = {requestName, friName, requestName};
        MariadbHelper::getInstance().execute(std::move(conn), "INSERT INTO user_friend (uid1, uid2, reqid) "
                        "VALUES"
                        " ((SELECT uid FROM user WHERE username=?), (SELECT uid FROM user WHERE username=?), (SELECT uid FROM user WHERE username=?));",
                        iparams);
        auto c = redis.execute("HGET %s fd", friName.c_str());
        if(c != std::nullopt)
        {
            for(auto &d : c.value())
            {
                if(std::stoi(d))
                {
                    //
                    VioletProtNeck neck = {};
                    strcpy(neck.command, "vafed");
                    memcpy(neck.name, requestName.c_str(), sizeof(neck.name));
                    std::string tmp("violet");
                    sr.sendMsg(std::stoi(d), neck, tmp);
                    std::cout<< "send add fri to fri success" <<std::endl;
                }
            }
        }
        MariadbHelper::getInstance().releaseConnection(std::move(conn));
        return 0;
    }
    else
    {
        MariadbHelper::getInstance().releaseConnection(std::move(conn));
        return -1;
    }
    MariadbHelper::getInstance().releaseConnection(std::move(conn));
    return -1;
}

int LoginCenter::vaddGroup(std::string reqName, std::string groupName, int fd)
{
    auto conn = MariadbHelper::getInstance().getConnection();
    // SELECT * FROM group_member WHERE (gid IN (SELECT gid FROM user_group WHERE gname=?)) AND (uid IN (SELECT uid FROM user WHERE username=?));
    std::vector<sql::SQLString> params = {groupName, reqName};
    auto ret = MariadbHelper::getInstance().query(std::move(conn), "SELECT * FROM group_member WHERE (gid IN (SELECT gid FROM user_group WHERE gname=?)) "
                             "AND (uid IN (SELECT uid FROM user WHERE username=?));",
                             params);
    // std::cout<< "add g query ret: " << ret.size() <<std::endl;
    if (ret.size() == 1)
    {
        MariadbHelper::getInstance().releaseConnection(std::move(conn));
        return 0;
    }
    else if (ret.size() == 0)
    {
        std::vector<sql::SQLString> grpparams = {groupName};
        auto retfri = MariadbHelper::getInstance().query(std::move(conn), "SELECT gid FROM user_group WHERE gname=?;",grpparams);
        std::string grpuid;
        if(retfri.size() != 1)
        {
            MariadbHelper::getInstance().releaseConnection(std::move(conn));
            return 1;
        }
        else
        {
            // for next version, delete if next version is come
            for(const auto &it : retfri)
            {
                grpuid = std::string(it.at("gid").c_str());
            }
        }
        // std::cout<< "debug log add g insert: " << reqName << "--" << groupName <<std::endl;
        std::vector<sql::SQLString> iparams = {groupName, reqName};
        bool m_ret = MariadbHelper::getInstance().execute(std::move(conn), "INSERT INTO group_member (gid, uid) VALUES "
                                     "((SELECT gid FROM user_group WHERE gname=?),(SELECT uid FROM user WHERE username=?));",
                                     iparams);
        // std::cout<< "debug log add g insert: " << m_ret << "--" << groupName <<std::endl;
        updateOnlineGUMap(groupName, fd);
        MariadbHelper::getInstance().releaseConnection(std::move(conn));
        return 0;
    }
    else
    {
        MariadbHelper::getInstance().releaseConnection(std::move(conn));
        return -1;
    }
    MariadbHelper::getInstance().releaseConnection(std::move(conn));
    return -1;
}

int LoginCenter::vcreateGroup(std::string reqName, std::string groupName, int fd)
{
    auto conn = MariadbHelper::getInstance().getConnection();
    // SELECT * FROM group_member WHERE (gid IN (SELECT gid FROM user_group WHERE gname=?)) AND (uid IN (SELECT uid FROM user WHERE username=?));
    std::vector<sql::SQLString> params = {groupName};
    auto ret = MariadbHelper::getInstance().query(std::move(conn), "SELECT gname FROM user_group WHERE gname=?;", params);
    // std::cout<< "create g query ret: " << ret.size() <<std::endl;
    if (ret.size() == 1)
    {
        MariadbHelper::getInstance().releaseConnection(std::move(conn));
        return 1;
    }
    else if (ret.size() == 0)
    {
        // std::cout<< "debug log create g insert: " << reqName << "--" << groupName <<std::endl;
        std::vector<sql::SQLString> iparams = {groupName, reqName};
        bool m_ret = MariadbHelper::getInstance().execute(std::move(conn), "INSERT INTO user_group (gname, gowner) VALUES (?, (SELECT uid FROM user WHERE username=?));",
                                     iparams);
        if(!m_ret)
        {
            std::cout<< "error on create group insert" <<std::endl;
        }
        auto gid = MariadbHelper::getInstance().getLastInsertId(conn);
        if (gid)
        {
            // 备注：下次测试看这个函数是不是工作正常，正常的话可以直接修改sql插入id，不用查一遍表
            // 测试完就把这条注释删掉
            std::cout << "gid: " << gid << std::endl;
        }
        std::vector<sql::SQLString> xparams = {groupName, reqName};
        bool v_ret = MariadbHelper::getInstance().execute(std::move(conn), "INSERT INTO group_member (gid, uid, grole) VALUES ((SELECT gid FROM user_group WHERE gname=?), (SELECT uid FROM user WHERE username=?), 'owner');",
                                     iparams);
        // std::cout<< "debug log create g insert: " << m_ret << "--" << groupName <<std::endl;
        updateOnlineGUMap(groupName, fd);
        MariadbHelper::getInstance().releaseConnection(std::move(conn));
        return 0;
    }
    else
    {
        MariadbHelper::getInstance().releaseConnection(std::move(conn));
        return -1;
    }
    MariadbHelper::getInstance().releaseConnection(std::move(conn));
    return -1;
}

int LoginCenter::vprivateChat(std::string friName)
{
    auto ret = redis.execute("HGET %s fd", friName.c_str());
    if(ret != std::nullopt)
    {
        for(auto &it : ret.value())
        {
            if(std::stoi(it))
            {
                return std::stoi(it);
            }
        }
    }
    return -1;
}

void LoginCenter::vgroupChat(int fd, std::string requestName, std::string groupName, std::string content)
{
    std::cout<< "params: " << requestName << groupName <<std::endl;
    auto it = onlineGUMap.find(groupName);
    if(it != onlineGUMap.end())
    {
        //std::cout<< "find group: " << it->first <<std::endl;
        VioletProtNeck neck = {};
        strcpy(neck.command, "vgcb");
        memcpy(neck.name, groupName.c_str(), sizeof(neck.name));
        memcpy(neck.pass, requestName.c_str(), sizeof(neck.pass));
        std::set<int> &tmp = it->second;
        for (int it : tmp)
        {
            if(it != fd)
            {
                sr.sendMsg(it, neck, content);
            }
        }
    }
    else
    {
        VioletProtNeck neck = {};
        strcpy(neck.command, "vgcerr");
        memcpy(neck.name, requestName.c_str(), sizeof(neck.name));
        std::string tmp("group not found");
        sr.sendMsg(fd, neck, tmp);
    }
}

/**
 * @brief LoginCenter::vofflineHandle
 * @param fd
 * 下线要做的事情：1. 给好友发送下线通知 2. 将用户fd从群组在线删除
 * 再多维护一个在线用户列表
 */
void LoginCenter::vofflineHandle(int fd)
{
    std::string tmpname;
    std::string tmpcount;
    auto it = onlineUser.find(fd);
    if(it != onlineUser.end())
    {
        tmpname = it->second;
        std::cout<< "offline found: " << tmpname <<std::endl;
        onlineUser.erase(fd);
    }
    auto ouf = onlineUserFriend.find(tmpname);
    if(ouf != onlineUserFriend.end())
    {
        std::list<int> &tmplist = ouf->second;
        for(auto j=tmplist.begin(); j!=tmplist.end(); ++j)
        {

            VioletProtNeck neck = {};
            strcpy(neck.command, "vbol");
            memcpy(neck.name, tmpname.c_str(), sizeof(neck.name));
            std::string tmp("violet");
            sr.sendMsg(*j, neck, tmp);
            std::cout<< "send offline msg on offliehandle for: " << *j <<std::endl;
        }
    }
    onlineUserFriend.erase(tmpname);
    //auto git = onlineGUMap.find()
    std::string tmpgn = tmpname + "grp";
    auto ret3 = redis.execute("LLEN %s", tmpgn.c_str());
    if(ret3 == std::nullopt)
    {
        std::cout<< "redis execute error on vofflineHandle" <<std::endl;
    }
    else
    {
        //optional<vector<string>>
        for(auto &it : ret3.value())
        {
            std::cout<< "offline grp name ccutn: " << it <<std::endl;
            tmpcount = it;
        }
    }
    std::cout<< "offline tmpcount: " << tmpcount <<std::endl;
    auto ret4 = redis.execute("LRANGE %s 0 %s", tmpgn.c_str(), tmpcount.c_str());
    if(ret4 == std::nullopt)
    {
        std::cout<< "redis execute error on vofflineHandle" <<std::endl;
    }
    else
    {
        for(auto &it : ret4.value())
        {
            //onlineGUMap: map<string, set<int>>
            auto vit = onlineGUMap.find(it);
            if(vit != onlineGUMap.end())
            {
                std::cout<< "offline found vit: " << vit->first <<std::endl;
                std::set<int> &tmpset = vit->second;
                tmpset.erase(fd);
            }
        }
    }
    auto ret2 = redis.execute("DEL %s", tmpname.c_str());
    if(ret2 == std::nullopt)
    {
        std::cout<< "redis execute error on vofflineHandle" <<std::endl;
    }
    auto ret1 = redis.execute("DEL %s", tmpgn.c_str());
    if(ret1 == std::nullopt)
    {
        std::cout<< "redis execute error on vofflineHandle" <<std::endl;
    }
}

//
std::string LoginCenter::serializeTwoVector(const std::vector<std::string> &vec1, const std::vector<std::string> &vec2)
{
    std::ostringstream oss;
    uint32_t size1 = htonl(vec1.size());
    oss.write(reinterpret_cast<const char *>(&size1), sizeof(size1));
    for (const auto &str : vec1)
    {
        uint32_t len = htonl(str.size());
        oss.write(reinterpret_cast<const char *>(&len), sizeof(len));
        oss.write(str.data(), str.size());
    }
    uint32_t size2 = htonl(vec2.size());
    oss.write(reinterpret_cast<const char *>(&size2), sizeof(size2));
    for (const auto &str : vec2)
    {
        uint32_t len = htonl(str.size());
        oss.write(reinterpret_cast<const char *>(&len), sizeof(len));
        oss.write(str.data(), str.size());
    }
    return oss.str();
}

std::pair<std::vector<std::string>, std::vector<std::string>> LoginCenter::deserializeVectors(const std::string &data)
{
    std::vector<std::string> vec1, vec2;
    const char *ptr = data.data();
    const char *end = ptr + data.size();
    if (ptr + sizeof(uint32_t) > end)
    {
        return {vec1, vec2};
    }
    uint32_t size1;
    memcpy(&size1, ptr, sizeof(size1));
    ptr += sizeof(size1);
    size1 = ntohl(size1);
    vec1.reserve(size1);
    for (uint32_t i = 0; i < size1; ++i)
    {
        if (ptr + sizeof(uint32_t) > end)
            break;
        uint32_t len;
        memcpy(&len, ptr, sizeof(len));
        ptr += sizeof(len);
        len = ntohl(len);
        if (ptr + len > end)
            break;
        vec1.emplace_back(ptr, len);
        ptr += len;
    }
    if (ptr + sizeof(uint32_t) > end)
    {
        return {vec1, vec2};
        uint32_t size2;
        memcpy(&size2, ptr, sizeof(size2));
        ptr += sizeof(size2);
        size2 = ntohl(size2);
        vec2.reserve(size2);
        for (uint32_t i = 0; i < size2; ++i)
        {
            if (ptr + sizeof(uint32_t) > end)
                break;
            uint32_t len;
            memcpy(&len, ptr, sizeof(len));
            ptr += sizeof(len);
            len = ntohl(len);
            if (ptr + len > end)
                break;
            vec2.emplace_back(ptr, len);
            ptr += len;
        }
    }
    return {vec1, vec2};
}

/**
 * @brief LoginCenter::updateOnlineGUMap
 * @param key 群组名字
 * @param value 登录用户的fd
 */
void LoginCenter::updateOnlineGUMap(const std::string &key, int value)
{
    auto it = onlineGUMap.find(key);
    if(it != onlineGUMap.end())
    {
        std::set<int> &tmp = it->second;
        if(tmp.find(value) == tmp.end())
        {
            tmp.insert(value);
        }
    }
    else
    {
        std::set<int> newSet = {value};
        onlineGUMap[key] = newSet;
    }
}



