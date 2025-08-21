#include <iostream>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctime>
#include <cassert>

#include "server.h"

int main()
{
    Server serv;
    serv.startServer();
    return 0;
}
