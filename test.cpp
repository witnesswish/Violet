#include "mariadbhelper.h"
#include "redishelper.h"
#include <iostream>

using namespace std;

int main()
{
    RedisHelper redis;
    redis.connectRedis("127.0.0.1", 6379, "");
    redis.execute("set mykey hello");
    redis.execute("set violet wuuuuuu");
    cout<< redis.execute("get violet") <<endl;
    redis.execute("SET %s %s", "counter", "42");
    int value = std::stoi(redis.execute("GET counter"));
    std::cout << "Counter: " << value << std::endl; // 输出 "42"
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
