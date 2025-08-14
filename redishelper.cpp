#include "redishelper.h"

// #define REDIS_REPLY_STRING 1
// #define REDIS_REPLY_ARRAY 2
// #define REDIS_REPLY_INTEGER 3
// #define REDIS_REPLY_NIL 4
// #define REDIS_REPLY_STATUS 5
// #define REDIS_REPLY_ERROR 6
// #define REDIS_REPLY_DOUBLE 7
// #define REDIS_REPLY_BOOL 8
// #define REDIS_REPLY_MAP 9
// #define REDIS_REPLY_SET 10
// #define REDIS_REPLY_ATTR 11
// #define REDIS_REPLY_PUSH 12
// #define REDIS_REPLY_BIGNUM 13
// #define REDIS_REPLY_VERB 14

RedisHelper::RedisHelper() {}

RedisHelper::~RedisHelper() {}

void RedisHelper::connectRedis(const std::string &host,
                               int port,
                               const std::string &password,
                               int timeout_sec)
{
    disconnectRedis();
    struct timeval timeout = {timeout_sec, 0};
    context_ = redisConnectWithTimeout(host.c_str(), port, timeout);

    if (!context_ || context_->err)
    {
        throw std::runtime_error(
            "Redis connection failed: " + std::string(context_ ? context_->errstr : "Can't allocate context"));
    }
    if (!password.empty())
    {
        auto reply = static_cast<redisReply *>(redisCommand(context_, "AUTH %s", password.c_str()));
        if (!reply || reply->type == REDIS_REPLY_ERROR)
        {
            std::string err = reply ? reply->str : "Auth failed";
            freeReplyObject(reply);
            throw std::runtime_error("Redis auth failed: " + err);
        }
        freeReplyObject(reply);
    }
    std::cout << "connected to redis success" << std::endl;
}

void RedisHelper::disconnectRedis()
{
    if (context_)
    {
        redisFree(context_);
        context_ = nullptr;
    }
}

bool RedisHelper::isConnected() const
{
    return context_ && !context_->err;
}
// 这个是没有参数的版本
std::optional<std::vector<std::string>> RedisHelper::execute(const std::string &command)
{
    if (!context_)
    {
        // throw std::runtime_error("Not connected to Redis");
        std::cout << "Not connected to Redis" << std::endl;
        return std::nullopt;
    }
    redisReply *reply = static_cast<redisReply *>(redisCommand(context_, command.c_str()));
    if (!reply)
    {
        // throw std::runtime_error("Command execution failed: " + std::string(context_->errstr));
        std::cout << "Command execution failed: " << std::string(context_->errstr) << std::endl;
        freeReplyObject(reply);
        return std::nullopt;
    }
    std::vector<std::string> result;
    if (reply->type == REDIS_REPLY_STRING)
    {
        freeReplyObject(reply);
        return result = std::string(reply->str, reply->len);
    }
    if (reply->type == REDIS_REPLY_INTEGER)
    {
        freeReplyObject(reply);
        result = std::to_string(reply->integer);
    }
    if (reply->type == REDIS_REPLY_ERROR)
    {
        std::string err = reply->str ? reply->str : "Unknown error";
        freeReplyObject(reply);
        // throw std::runtime_error("Redis error: " + err);
        std::cout << "redis error: " << err << std::endl;
        freeReplyObject(reply);
        return std::nullopt;
    }
    if (reply->type == REDIS_REPLY_NIL)
    {
        freeReplyObject(reply);
        return result = "";
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
    // 根据打印出来的值判断吧，遇到一个加一个，定义我贴到这个文件顶部了，debug的时候看这个就够了
    std::cout << "undefined reply type: " << reply->type << std::endl;
    freeReplyObject(reply);
    return std::nullopt;
}
