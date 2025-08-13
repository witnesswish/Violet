#ifndef REDISHELPER_H
#define REDISHELPER_H

#include <hiredis/hiredis.h>
#include <stdexcept>
#include <string>
#include <vector>

class RedisHelper
{
public:
    RedisHelper();
    ~RedisHelper();

public:
    void connectRedis(const std::string &host = "127.0.0.1",
                      int port = 6379,
                      const std::string &password = "",
                      int timeout_sec = 1);
    void disconnectRedis();
    bool isConnected() const;
    std::string execute(const std::string& command);
    template<typename... Args>
    std::string execute(const std::string& format, Args... args);

private:
        redisContext *context_;
};

#endif // REDISHELPER_H
