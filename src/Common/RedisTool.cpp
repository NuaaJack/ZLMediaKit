//
// Created by 曹旭峰 on 2024/6/27.
//
#include "RedisTool.h"
#include "Util/logger.h"

using namespace toolkit;

namespace mediakit {
    RedisTool::RedisTool() : _connect(nullptr), _port(0), _database_id(-1) {

    }

    RedisTool::~RedisTool() {
        _connect = nullptr;
    }

    bool RedisTool::connect(string host, int port, string pwd, int database_id) {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_connect != nullptr) {
            InfoL << "Redis _connect ! = nullptr";
            return true;
        }
        if (host.empty()) {
            InfoL << "Redis host == nullptr";
            return false;
        }

        struct timeval timeout = {2, 0};
        _connect = redisConnectWithTimeout(host.c_str(), port, timeout);
        if (_connect == nullptr) {
            ErrorL << "Redis Error: Conenct Failed, Check ip and port";
            return false;
        } else if (_connect->err != 0) {
            ErrorL << "Redis Error: Conenct Error: " << _connect->errstr;
            redisFree(_connect);
	        _connect = nullptr;
	        return false;
        }

	 InfoL << "Redis Conenct ok"; 
        if (!pwd.empty()) {
            redisReply *reply = (redisReply *) redisCommand(_connect, "AUTH %s", pwd.c_str());
            if (reply == nullptr) {
                ErrorL << "Redis Error: AUTH command failed";
                return false;
            }
            if (reply->type == REDIS_REPLY_ERROR) {
                ErrorL << "Redis Error: Conenct Password maybe Error";
                freeReplyObject(reply);
                return false;
            }
            freeReplyObject(reply);
        }

        if(database_id >= 0){
            string database = to_string(database_id);
            redisReply *reply = (redisReply *) redisCommand(_connect, "SELECT %s", database.c_str());
            if (reply == nullptr) {
                ErrorL << "Redis Error: SELECT command failed";
                return false;
            }
            if (reply->type == REDIS_REPLY_ERROR) {
                ErrorL << "Redis Error: Select database " << database_id << "maybe Error";
                freeReplyObject(reply);
                return false;
            }
            InfoL << " Redis : Select database " << database_id;
            freeReplyObject(reply);
        }
        _host = host;
        _port = port;
        _pwd = pwd;
        _database_id = database_id;
        return true;
    }

    void RedisTool::disconnect() {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_connect) {
            redisFree(_connect);
            _connect = nullptr;
        }
    }

    bool RedisTool::reconnect(){
        disconnect();
        sleep(1);
        if (_host.empty()) {
            return false;
        }
        InfoL << "Reconnect Redis host:" << _host << ", Port :" << _port << ", Database Id:" << _database_id;
        return connect(_host, _port, _pwd, _database_id);
    }

    int RedisTool::redis_get(string key, string &value) {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_connect == nullptr || key.empty()) {
            InfoL << "Redis Connect is not ready!!";
            return -1;
        }
        //value.clear();
	    std::string redis_ret; 
        InfoL << "redisCommand parma key =" << key;
       // redisReply *reply = redisCommand(_connect, "GET key:%s", key.c_str());
	    redisReply *reply = (redisReply *) redisCommand(_connect, "GET %s", key.c_str());
        if (reply == nullptr) {
            ErrorL << "Redis Error: Get key " << key << " return result is null";
            return -1;
        }else if(reply->type != REDIS_REPLY_STRING && reply->type != REDIS_REPLY_NIL){
            ErrorL << "Redis Error: Get key " << key << " return result is not string type";
            freeReplyObject(reply);
            return -1;
        } else if (reply->type == REDIS_REPLY_NIL) {
            freeReplyObject(reply);
            //有时候需要查不到，这里返回1，和-1进行区分
            return 1;
        }
        redis_ret = reply->str;
        if(redis_ret.empty()){
            freeReplyObject(reply);
            //有时候需要查不到，这里返回1，和-1进行区分
            return 1;
        }
        if(reply != nullptr){
            freeReplyObject(reply);
        }
	value = redis_ret;
        return 0;
    }

    int RedisTool::redis_set(std::string key, std::string value) {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_connect == nullptr) {
            ErrorL << "Redis Error: Redis Connect is not extablished";
            return -1;
        } else if (key.length() <= 0 || value.length() <= 0) {
            ErrorL << "Redis Error: key or value is empty";
            return -1;
        }

        redisReply *reply = (redisReply *) redisCommand(_connect, "SET %s %s", key.c_str(), value.c_str());
        if (reply == nullptr) {
            ErrorL << "Redis Error: SET error!";
            return -1;
        }
        if (!(reply->type == REDIS_REPLY_STATUS && memcmp(reply->str, "OK", 2) == 0)) {
            ErrorL << "Redis Error: SET error!";
            freeReplyObject(reply);
            return -1;
        }
        freeReplyObject(reply);
        InfoL << "Redis set: "<< key <<" value: "<< value <<" success!";
        return 0;
    }

    int RedisTool::redis_delete(string key) {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_connect == nullptr) {
            ErrorL << "Redis Error: Redis Connect is not connected";
            return -1;
        }

        redisReply *reply = (redisReply *) redisCommand(_connect, "DEL %s", key.c_str());
        if (reply == nullptr) {
            ErrorL << "Redis Error: DEL command failed!";
            return -1;
        }
        if (reply->type != REDIS_REPLY_INTEGER) {
            ErrorL << "Redis Error: The key doesn't exist!";
            freeReplyObject(reply);
            return -1;
        }
        int ret;
        if (reply->integer == 0){
            ret = 1;
        }else{
            ret = reply->integer - 1;
        }

        freeReplyObject(reply);
        InfoL << "Redis del key: " << key << " success!";
        return ret;
    }

    bool RedisTool::redis_key_exist(string key){
        std::lock_guard<std::mutex> lock(_mutex);
        if (_connect == nullptr) {
            ErrorL << "Redis Error: Redis Connect is not connected";
            return -1;
        }
        redisReply *reply = (redisReply *) redisCommand(_connect, "keys %s", key.c_str());
        if (reply == nullptr) {
            ErrorL << "Redis Error: keys command failed!";
            return false;
        }
        if (reply->type != REDIS_REPLY_INTEGER) {
            ErrorL << "The key: "<< key <<" doesn't exist!";
            freeReplyObject(reply);
            return false;
        }
        return true;
    }
}
