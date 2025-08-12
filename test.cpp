#include "mariadbhelper.h"
#include <iostream>

int main()
{
    MariadbHelper mariadb("violet", "violet@123", "violet");
    if (mariadb.connectMariadb() < 0) {
        return -1;
    }
    /**
    std::vector<sql::SQLString> params = {
        "user1",
        "user1",
        "uv1@elveso.asia",
        "user1",
        "salt"
    };
    mariadb.execute(
        "insert into user (username, nickname, email, password, salt) values (?, ?, ?, ?, ?);", params);
    **/
    std::vector<sql::SQLString> params = {"1"};
    auto ret = mariadb.query("select username, email from user where 1=?;", params);
    for(const auto& row : ret)
    {
        std::cout<< row.at("username") << "--" << row.at("email") <<std::endl;
        if(row.at("username").c_str() == (const char*)"user1")
        {
            std::cout<< "1" <<std::endl;
        }
        else
        {
            std::cout<< "2" <<std::endl;
        }
    }
    return 0;
}
