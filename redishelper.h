#ifndef REDISHELPER_H
#define REDISHELPER_H

#include <hiredis/hiredis.h>
#include <stdexcept>
#include <string>
#include <vector>
#include <iostream>
#include <optional>

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
    std::optional<std::vector<std::string>> execute(const std::string &command);
    template <typename... Args>
    std::optional<std::vector<std::string>> execute(const std::string &format, Args... args)
    {
        if (!context_)
        {
            // throw std::runtime_error("Not connected to Redis");
            std::cout << "not connected to redis" << std::endl;
            return std::nullopt;
        }
        redisReply *reply = static_cast<redisReply *>(redisCommand(context_, format.c_str(), args...));
        if (!reply)
        {
            // throw std::runtime_error("Command execution failed: " + std::string(context_->errstr));
            std::cout << "Command execution failed" << std::string(context_->errstr) << std::endl;
            freeReplyObject(reply);
            return std::nullopt;
        }
        std::vector<std::string> result;
        if (reply->type == REDIS_REPLY_STRING)
        {
            return result.push_back(std::string(reply->str, reply->len));
        }
        if (reply->type == REDIS_REPLY_INTEGER)
        {
            result.push_back(std::to_string(reply->integer));
        }
        if (reply->type == REDIS_REPLY_ERROR)
        {
            std::string err = reply->str ? reply->str : "Unknown error";
            freeReplyObject(reply);
            // throw std::runtime_error("Redis error: " + err);
            std::cout << "redis error: " << err << std::end;
            freeReplyObject(reply);
            return std::nullopt;
        }
        if (reply->type == REDIS_REPLY_NIL)
        {
            freeReplyObject(reply);
            return std::nullopt;
        }
        if (reply->type == REDIS_REPLY_ARRAY)
        {
            for (size_t i = 0; i < reply->; ++i)
            {
                redisReply *childReply = reply->element[i];
                if (childreply->type = REDIS_REPLY_STRING)
                {
                    result.emplace_back(childReply->str, childReply->len);
                }
                else if (childReply->type == REDIS_REPLY_INTEGER)
                {
                    result.emplace_back(std::to_string(childReply->integer));
                }
                else
                {
                    std::cout << "redis reply child reply return: " << childReply->type << std::endl;
                }
            }
            if (result.size() == 0)
            {
                freeReplyObject(reply);
                return std::nullopt;
            }
            return result;
        }
        // freeReplyObject(reply);
        // throw std::runtime_error("Unsupported reply type");
        std::cout << "undefined reply type" << reply->type << std::endl;
        freeReplyObject(reply);
        return std::nullopt;
    }

private:
    redisContext *context_;
};

#endif // REDISHELPER_H
