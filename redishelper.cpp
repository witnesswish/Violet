#include "redishelper.h"

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

    if (!context_ || context_->err) {
        throw std::runtime_error(
                    "Redis connection failed: "
                    + std::string(context_ ? context_->errstr : "Can't allocate context"));
    }
    if (!password.empty())
    {
        auto reply = static_cast<redisReply *>(redisCommand(context_, "AUTH %s", password.c_str()));
        if (!reply || reply->type == REDIS_REPLY_ERROR) {
            std::string err = reply ? reply->str : "Auth failed";
            freeReplyObject(reply);
            throw std::runtime_error("Redis auth failed: " + err);
        }
        freeReplyObject(reply);
    }
}

void RedisHelper::disconnectRedis()
{
    if (context_) {
        redisFree(context_);
        context_ = nullptr;
    }
}

bool RedisHelper::isConnected() const
{
    return context_ && !context_->err;
}

std::string RedisHelper::execute(const std::string &command)
{
    if (!context_)
    {
        throw std::runtime_error("Not connected to Redis");
    }
    redisReply* reply = static_cast<redisReply*>(
                redisCommand(context_, command.c_str())
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
        std::cout<< "undefined reply type: " << reply->type <<std::endl;
    }

    freeReplyObject(reply);
    return result;
}

