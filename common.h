#ifndef COMMON_H
#define COMMON_H

#include <iostream>

struct Msg
{
    int magic;
    int type;
    int plateform;
    std::string content;
};

#endif // COMMON_H
