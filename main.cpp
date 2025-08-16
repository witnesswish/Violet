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

// int main()
// {
//     pid_t pid = fork();
//     if (pid < 0)
//     {
//         exit(1);
//     } else if (pid > 0)
//     {
//         exit(0);
//     }

//     setsid();
//     chdir("/");
//     umask(0);

//     for (int i = 0; i < getdtablesize(); i++) {
//         close(i);
//     }

//     while (true) {
//         FILE* fp = fopen("/tmp/mytest.log", "a");
//         assert(fp != NULL);
//         time_t rawtime;
//         struct tm* timeinfo;
//         time(&rawtime);
//         timeinfo = localtime(&rawtime);
//         fprintf(fp, "守护进程测试：%02d-%02d-%02d %02d:%02d:%02d\n",
//                 timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday,
//                 timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
//         fclose(fp);
//         sleep(5);
//     }

//     return 0;
// }
