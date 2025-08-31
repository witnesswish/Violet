#include "common.h"

std::unordered_map<int, SSL*> fdSslMap;
std::mutex fdSslMapMutex;
