//
// Created by 曹旭峰 on 2021/8/16.
//

#ifndef ZLMEDIAKIT_REDISTOOL_H
#define ZLMEDIAKIT_REDISTOOL_H

#include <iostream>
#include <string.h>
#include <string>
#include <stdio.h>
#include <mutex>
#include <memory>
#include <vector>
#include <hiredis/hiredis.h>
#include <hiredis/async.h>

using namespace std;

namespace mediakit {
    class RedisTool : public enable_shared_from_this<RedisTool> {
    public:
        typedef std::shared_ptr<RedisTool> Ptr;

        RedisTool();

        virtual ~RedisTool();

        //连接
        bool connect(string host, int port, string pwd, int database_id = 0);

        //断开连接
        void disconnect();

        bool reconnect();

        //redis get
        int redis_get(string key, string &value);

        //redis set
        int redis_set(std::string key, std::string value);

        //redis delete
        int redis_delete(string key);

        //key is exist
        bool redis_key_exist(string key);

    private:
        redisReply* executeCommand(const std::vector<std::string>& cmd) {
            std::lock_guard<std::mutex> lock(_mutex);
            int argc = static_cast<int>(cmd.size());
            std::vector<const char*> argv(argc);
            std::vector<size_t> argv_len(argc);

            for (int i = 0; i < argc; ++i) {
                argv[i] = cmd[i].c_str();
                argv_len[i] = cmd[i].size();
            }

            redisReply* reply = static_cast<redisReply*>(redisCommandArgv( _connect, argc, argv.data(), argv_len.data()));
            return reply;
        }

    private:
        redisContext *_connect;
        string _host;
        int _port;
        string _pwd;
        int _database_id;
        mutex _mutex;
    };
}
#endif //ZLMEDIAKIT_REDISTOOL_H
