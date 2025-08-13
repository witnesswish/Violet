#ifndef REDISHELPER_H
#define REDISHELPER_H

#include <hiredis/hiredis.h>
#include <stdexcept>
#include <string>
#include <vector>
#include <iostream>

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
std::string execute(const std::string &format, Args... args)
{
    if (!context_) {
        throw std::runtime_error("Not connected to Redis");
    }

    redisReply* reply = static_cast<redisReply*>(
                redisCommand(context_, format.c_str(), args...)
                );
    if (!reply) {
        throw std::runtime_error("Command execution failed: " +
                                 std::string(context_->errstr));
    }
    std::string result;
    if (reply->type == REDIS_REPLY_STRING) {
        result = std::string(reply->str, reply->len);
    } else if (reply->type == REDIS_REPLY_INTEGER) {
        result = std::to_string(reply->integer);
    } else if (reply->type == REDIS_REPLY_ERROR) {
        std::string err = reply->str ? reply->str : "Unknown error";
        freeReplyObject(reply);
        throw std::runtime_error("Redis error: " + err);
    } else if (reply->type == REDIS_REPLY_NIL) {
        result = "";
    } else {
        //freeReplyObject(reply);
        //throw std::runtime_error("Unsupported reply type");
        std::cout<< "unsupported reply type" <<std::endl;
    }

    freeReplyObject(reply);
    return result;
}

private:
        redisContext *context_;
};

#endif // REDISHELPER_H
