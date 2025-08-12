#include "logincenter.h"

LoginCenter::LoginCenter()
    : mariadb(setMariadb().m_user,
              setMariadb().m_password,
              setMariadb().m_database,
              setMariadb().m_host,
              setMariadb().m_port,
              false)
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
    std::vector<sql::SQLString> params = {username};
    auto ret = mariadb.qeury("select username from user where username=?;", username);
    if (ret.size() > 1 || ret.size() != 0) {
        return 1;
    }
    std::vector<sql::SQLString> params = {username, email, password, salt};
    mariadb.execute(
        "insert into user (username, nickname, email, password, salt) values (?, ?, ?, ?, ?);",
        params);
    return 0;
}

/**
 * @brief LoginCenter::login
 * @param username
 * @param password
 * @return -1 for database error
 * 0 for succ, 1 for username not exists, 2 for password error
 */
int LoginCenter::login(std::string username, std::string password, User &uerinfo)
{
    User u;
    if (mariadb.connectMariadb() < 0) {
        return -1;
    }
    std::vector<sql::SQLString> params = {username};
    auto ret = mariadb.qeury(
        "select username, nickname, password, salt, avat from user where username=?;", username);
    if (ret.size() < 1 || ret.size() != 1) {
        return 1;
    }
    for (const auto &row : ret) {
        //std::cout<< row.at("username") << "--" << row.at("email") <<std::endl;
        if (password != ret.at("password")) {
            return 2;
        }
    }
    //到此为止，登录校验已经完成，下面是把用户的群组好友等信息返回
    std::vector<sql::SQLString> params = {username};
    auto friInfo = mariadb.qeury("SELECT DISTINCT u.username AS friend_name FROM user_friend f "
                                 "JOIN user u ON f.uid2=u.uid OR f.uid1=u.uid "
                                 "WHERE f.uid1 IN (SELECT uid FROM user WHERE username=?) "
                                 "OR f.uid2 IN ( SELECT uid FROM user WHERE username=?);",
                                 params);
    for (const auto &row : friInfo) {
        if (username != ret.at("friend_name")) {
            u.friends.push_back(ret.at("friend_name"));
        }
    }
    auto groupInfo = mariadb.query(
        "SELECT DISTINCT g.gname AS group_name FROM user_group g JOIN group_member gm "
        "WHERE gm.gid=g.gid AND gm.uid IN (SELECT uid FROM user WHERE username=?);",
        params);
    for (const auto &row : friInfo) {
        u.groups.push_back(ret.at("group_name"));
    }
    mariadb.disconnectMariadb();
    return 0;
}

Madb LoginCenter::setMariadb()
{
    Madb m;
    m.m_user = "violet";
    m.m_password = "violet@1admin";
    m.m_database = "violet";
    m.m_host = "127.0.0.1";
    m.m_port = 3434;
    return m;
}
