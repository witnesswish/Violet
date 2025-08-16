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
            std::string tmp = std::string(reply->str, reply->len);
            result.push_back(tmp);
            freeReplyObject(reply);
            return result;
        }
        if (reply->type == REDIS_REPLY_INTEGER)
        {
            std::cout<< "read interger from redis: " << reply->integer <<std::endl;
            std::string intmp = std::to_string(reply->integer);
            result.push_back(intmp);
            freeReplyObject(reply);
            return result;
        }
        if (reply->type == REDIS_REPLY_ERROR)
        {
            std::string err = reply->str ? reply->str : "Unknown error";
            // throw std::runtime_error("Redis error: " + err);
            std::cout << "redis error: " << err << std::endl;
            freeReplyObject(reply);
            return std::nullopt;
        }
        if (reply->type == REDIS_REPLY_NIL)
        {
            std::cout<< "read nil from redis, close it" <<std::endl;
            freeReplyObject(reply);
            return std::nullopt;
        }
        if(reply->type == REDIS_REPLY_STATUS)
        {
            std::string vstatus(reply->str, reply->len);
            std::cout<< "read status from redis: " << vstatus <<std::endl;
            result.push_back(vstatus);
            freeReplyObject(reply);
            return result;
        }
        if (reply->type == REDIS_REPLY_ARRAY)
        {
            std::cout<< "read array from redis, array num: " << reply->elements <<std::endl;
            for (size_t i = 0; i < reply->elements; ++i)
            {
                redisReply *childReply = reply->element[i];
                if (childReply->type = REDIS_REPLY_STRING)
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
            freeReplyObject(reply);
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
