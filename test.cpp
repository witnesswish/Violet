#include "mariadbhelper.h"
#include "redishelper.h"
#include <iostream>
#include <vector>
#include <optional>
#include <string>

using namespace std;

int main()
{
    RedisHelper redis;
    redis.connectRedis("127.0.0.1", 6379);
    redis.execute("sadd test test1 test2");
    auto ret1 = redis.execute("smembers test");
    auto ret2 = redis.execute("sismember test test77");
    if(ret1 != nullopt)
    {
        for(auto it : ret1.value())
        {
                cout<< it <<endl;
        }
        for(auto it : ret2.value())
        {
                cout<< it <<endl;
        }
    }
    redis.execute("hmset testuser username test uid 5 fd 45");
    auto ret3 = redis.execute("hget testuser fd");
    for(auto it : ret3.value())
    {
            cout<< it <<endl;
    }
    redis.disconnectRedis();
    return 0;
}

// int main()
// {
//     MariadbHelper mariadb("violet", "violet@123", "violet");
//     if (mariadb.connectMariadb() < 0) {
//         return -1;
//     }
//     /**
//     std::vector<sql::SQLString> params = {
//         "user1",
//         "user1",
//         "uv1@elveso.asia",
//         "user1",
//         "salt"
//     };
//     mariadb.execute(
//         "insert into user (username, nickname, email, password, salt) values (?, ?, ?, ?, ?);", params);
//     **/
//     std::vector<sql::SQLString> params = {"1"};
//     auto ret = mariadb.query("select username, email from user where 1=?;", params);
//     for(const auto& row : ret)
//     {
//         std::cout<< row.at("username") << "--" << row.at("email") <<std::endl;
//         if(row.at("username").c_str() == (const char*)"user1")
//         {
//             std::cout<< "1" <<std::endl;
//         }
//         else
//         {
//             std::cout<< "2" <<std::endl;
//         }
//     }
//     return 0;
// }
