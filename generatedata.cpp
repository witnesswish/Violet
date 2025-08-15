// #include "mariadbhelper.h"
// #include "redishelper.h"
#include <iostream>
// #include <vector>
// #include <optional>
// #include <string>

// using namespace std;

// int main()
// {
//     MariadbHelper mariadb("violet", "violet@123", "violet");
//     if (mariadb.connectMariadb() < 0) {
//         return -1;
//     }
//     std::vector<sql::SQLString> params = {"violet"};
//     mariadb.execute(
//         "SELECT CONCAT('TRUNCATE TABLE `', TABLE_NAME, '`;') FROM INFORMATION_SCHEMA.TABLES WHERE TABLE_SCHEMA = ?;", params);
//     for(int i=0; i<5; ++i)
//     {
//         params = {"user" + to_string(i), "user" + to_string(i), to_string(i), "user" + to_string(i), "user" + to_string(i)};
//         mariadb.execute(
//             "INSERT INTO user_group (gname, g_description, owner) VALUES (?, ?, ?);", params);
//         params = {"test" + to_string(i), "测试群", to_string(i)};
//         mariadb.execute(
//             "INSERT INTO user_group (gname, g_description, owner) VALUES (?, ?, ?);", params);
//     }
//     mariadb.disconnectMariadb();
//     return 0;
// }
int main()
{
    std::cout<< "hello" <<std::endl;
    return 0;
}
