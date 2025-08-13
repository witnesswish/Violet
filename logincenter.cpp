#include "logincenter.h"

LoginCenter::LoginCenter()
    : mariadb(setMariadb().m_user,
              setMariadb().m_password,
              setMariadb().m_database,
              setMariadb().m_host,
              setMariadb().m_port,
              true)
{}

/**
 * @brief LoginCenter::vregister
 * @param username
 * @param password
 * @return 0 for normal, 1 for user exists
 */
int LoginCenter::vregister(std::string username,
                           std::string password,
                           std::string email,
                           std::string salt)
{
    std::vector<sql::SQLString> params = {username.c_str()};
    auto ret = mariadb.query("select username from user where username=?;", params);
    if (ret.size() > 1 || ret.size() != 0) {
        return 1;
    }
    std::vector<sql::SQLString> iparams = {username, username, email, password, salt};
    mariadb.execute(
                "insert into user (username, nickname, email, password, salt) values (?, ?, ?, ?, ?);",
                iparams);
    return 0;
}

/**
 * @brief LoginCenter::login
 * @param username
 * @param password
 * @return -1 for database error
 * 0 for succ, 1 for username not exists, 2 for password error
 */
int LoginCenter::vlogin(std::string username, std::string password, std::string &userinfo)
{
    User u;
    if (mariadb.connectMariadb() < 0) {
        return -1;
    }
    std::vector<sql::SQLString> params = {username};
    auto ret = mariadb.query(
                "select username, nickname, password, salt, avat from user where username=?;", params);
    //如果查出来的数据不是一条或者0条，那就是数据库大概率出问题了
    if (ret.size() != 1) {
        return 1;
    }
    for (const auto &row : ret) {
        //vector<map<string, sql::SQLString>> query
        if (password != std::string(row.at("password").c_str())) {
            return 2;
        }
    }
    //到此为止，登录逻辑已经完成，下面是把用户的群组好友等信息返回
    //firend
    std::vector<sql::SQLString> fparams = {username, username};
    auto friInfo = mariadb.query("SELECT DISTINCT u.username AS friend_name FROM user_friend f "
                                 "JOIN user u ON f.uid2=u.uid OR f.uid1=u.uid "
                                 "WHERE f.uid1 IN (SELECT uid FROM user WHERE username=?) "
                                 "OR f.uid2 IN ( SELECT uid FROM user WHERE username=?);",
                                 fparams);
    for (const auto& irow : friInfo) {
        if (username != std::string(irow.at("username").c_str())) {
            u.friends.push_back(std::string(irow.at("username")));
            std::cout<< irow.at("username") <<std::endl;
        }
    }
    auto groupInfo = mariadb.query(
                "SELECT DISTINCT g.gname AS group_name FROM user_group g JOIN group_member gm "
                "WHERE gm.gid=g.gid AND gm.uid IN (SELECT uid FROM user WHERE username=?);",
                params);
    for (const auto &row : groupInfo) {
        u.groups.push_back(std::string(row.at("gname")));
    }
    mariadb.disconnectMariadb();
    userinfo = serializeTwoVector(u.friends, u.groups);
    //std::cout<< "sizeof info: " << userinfo <<std::endl;
    return 0;
}

/**
 * @brief LoginCenter::vaddFriend
 * @param requestName
 * @param friName
 * @return
 * 理论上来说插入好友关系的时候，应该固定好小的id在前还是大的id在前，不过没关系啦，我写的sql够复杂
 * 我写这段话只是希望下次我看到的时候把这里改一下，这样应该好查一点
 */
int LoginCenter::vaddFriend(std::string requestName, std::string friName)
{
    if (mariadb.connectMariadb() < 0) {
        return -1;
    }
    std::vector<sql::SQLString> params = {requestName, requestName, friName, friName};
    auto ret = mariadb.query("SELECT * FROM user_friend uf WHERE (uf.uid1 IN (SELECT uid FROM user WHERE username=?)) "
                             "OR (uf.uid1 IN (SELECT uid FROM user WHERE username=?)) AND (uf.uid1 IN (SELECT uid FROM user "
                             "WHERE username=?)) OR (uf.uid1 IN (SELECT uid FROM user WHERE username=?))", params);
    //std::cout<< "add f: " << ret.size() <<std::endl;
    if(ret.size() == 1)
    {
        return 0;
    }
    else if(ret.size() == 0)
    {
        std::vector<sql::SQLString> iparams = {requestName, friName, requestName};
        mariadb.execute("INSERT INTO user_friend (uid1, uid2, reqid) "
                        "VALUES"
                        " ((SELECT uid FROM user WHERE username=?), (SELECT uid FROM user WHERE username=?), (SELECT uid FROM user WHERE username=?));",
                    iparams);
        return 0;
    }
    else
    {
        return -1;
    }
    return -1;
}

int LoginCenter::vaddGroup(std::string reqName, std::string groupName)
{
    if (mariadb.connectMariadb() < 0) {
        return -1;
    }
    //SELECT * FROM group_member WHERE (gid IN (SELECT gid FROM user_group WHERE gname=?)) AND (uid IN (SELECT uid FROM user WHERE username=?));
    std::vector<sql::SQLString> params = {reqName, groupName};
    auto ret = mariadb.query("SELECT * FROM group_member WHERE (gid IN (SELECT gid FROM user_group WHERE gname=?)) "
                             "AND (uid IN (SELECT uid FROM user WHERE username=?));", params);
    //std::cout<< "add g query ret: " << ret.size() <<std::endl;
    if(ret.size() == 1)
    {
        return 1;
    }
    else if(ret.size() == 0)
    {
        //std::cout<< "debug log add g insert: " << reqName << "--" << groupName <<std::endl;
        std::vector<sql::SQLString> iparams = {groupName, reqName};
        bool m_ret = mariadb.execute("INSERT INTO group_member (gid, uid) VALUES "
                        "((SELECT gid FROM user_group WHERE gname=?),(SELECT uid FROM user WHERE username=?));",
                    iparams);
        //std::cout<< "debug log add g insert: " << m_ret << "--" << groupName <<std::endl;
        return 0;
    }
    else
    {
        return -1;
    }
    return -1;
}

int LoginCenter::vcreateGroup(std::string reqName, std::string groupName)
{
    if (mariadb.connectMariadb() < 0) {
        return -1;
    }
    //SELECT * FROM group_member WHERE (gid IN (SELECT gid FROM user_group WHERE gname=?)) AND (uid IN (SELECT uid FROM user WHERE username=?));
    std::vector<sql::SQLString> params = {groupName};
    auto ret = mariadb.query("SELECT gname FROM user_group WHERE gname=?;", params);
    //std::cout<< "create g query ret: " << ret.size() <<std::endl;
    if(ret.size() == 1)
    {
        return 1;
    }
    else if(ret.size() == 0)
    {
        //std::cout<< "debug log create g insert: " << reqName << "--" << groupName <<std::endl;
        std::vector<sql::SQLString> iparams = {groupName, reqName};
        bool m_ret = mariadb.execute("INSERT INTO user_group (gname, gowner) VALUES (?, (SELECT uid FROM user WHERE username=?));",
                    iparams);
        auto gid = mariadb.getLastInsertId();
        if(gid == 0)
        {
            //备注：下次测试看这个函数是不是工作正常，正常的话可以直接修改sql插入id，不用查一遍表
            //测试完就把这条注释删掉
            std::cout<< "gid: " << gid <<std::end;
        }
        std::vector<sql::SQLString> xparams = {groupName, reqName};
        bool v_ret = mariadb.execute("INSERT INTO group_member (gid, uid) VALUES ((SELECT gid FROM user_group WHERE gname=?), (SELECT uid FROM user WHERE username=?));",
                    iparams);
        //std::cout<< "debug log create g insert: " << m_ret << "--" << groupName <<std::endl;
        return 0;
    }
    else
    {
        return -1;
    }
    return -1;
}

Madb LoginCenter::setMariadb()
{
    Madb m;
    m.m_user = "violet";
    m.m_password = "violet@123";
    m.m_database = "violet";
    m.m_host = "127.0.0.1";
    m.m_port = 3306;
    return m;
}

//
std::string LoginCenter::serializeTwoVector(const std::vector<std::string> &vec1, const std::vector<std::string> &vec2)
{
    std::ostringstream oss;
    uint32_t size1 = htonl(vec1.size());
    oss.write(reinterpret_cast<const char*>(&size1), sizeof(size1));
    for(const auto &str : vec1)
    {
        uint32_t len = htonl(str.size());
        oss.write(reinterpret_cast<const char*>(&len), sizeof(len));
        oss.write(str.data(), str.size());
    }
    uint32_t size2 = htonl(vec2.size());
    oss.write(reinterpret_cast<const char*>(&size2), sizeof(size2));
    for(const auto &str : vec2)
    {
        uint32_t len = htonl(str.size());
        oss.write(reinterpret_cast<const char*>(&len), sizeof(len));
        oss.write(str.data(), str.size());
    }
    return oss.str();
}

std::pair<std::vector<std::string>, std::vector<std::string> > LoginCenter::deserializeVectors(const std::string &data)
{
    std::vector<std::string> vec1, vec2;
    const char* ptr = data.data();
    const char* end = ptr + data.size();
    if(ptr + sizeof(uint32_t) > end)
    {
        return {vec1, vec2};
    }
    uint32_t size1;
    memcpy(&size1, ptr, sizeof(size1));
    ptr += sizeof(size1);
    size1 = ntohl(size1);
    vec1.reserve(size1);
    for(uint32_t i=0; i<size1; ++i)
    {
        if (ptr + sizeof(uint32_t) > end)
            break;
        uint32_t len;
        memcpy(&len, ptr, sizeof(len));
        ptr += sizeof(len);
        len = ntohl(len);
        if (ptr + len > end) break;
        vec1.emplace_back(ptr, len);
        ptr += len;
    }
    if(ptr + sizeof(uint32_t) > end)
    {
        return {vec1, vec2};
        uint32_t size2;
        memcpy(&size2, ptr, sizeof(size2));
        ptr += sizeof(size2);
        size2 = ntohl(size2);
        vec2.reserve(size2);
        for(uint32_t i=0; i<size2; ++i)
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
