#include "mariadbhelper.h"
#include <iostream>

int main()
{
    MariadbHelper mariadb;
    if (mariadb.connectMariadb("violet", "", "violet") < 0) {
        return -1;
    }
    std::vector<sql::SQLString> params = {
        "user1",
        "user1",
        "uv1@elveso.asia",
        "user1",
        "salt"
    };
    mariadb.execute(
        "insert into user (username, nickname, email, password, salt) values (?, ?, ?, ?, ?);", params);
    mariadb.query();
    reteurn 0;
}
