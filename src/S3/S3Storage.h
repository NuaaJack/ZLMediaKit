//
// Created by 曹旭峰 on 2021/9/5.
//

#ifndef ZLMEDIAKIT_S3STORAGE_H
#define ZLMEDIAKIT_S3STORAGE_H

#if defined(ENABLE_S3_STORAGE)
#include <string>
#include <vector>
#include <time.h>
#include "SafeAwsInclude.h"

using std::string;
using std::vector;
using Aws::S3::S3Client;

namespace mediakit{

    class S3Storage {
    public:
        S3Storage(const S3Storage&)=delete;
        S3Storage& operator=(const S3Storage&)=delete;

        static S3Storage& getInstance(){
            static S3Storage m_instance;  //局部静态变量
            return m_instance;
        }

        virtual ~S3Storage();

        bool InitS3(string endpoint, string access_id, string access_secret, bool https);

        /**
         * create bucket
         * @param bucket_name
         * @return
         */
        bool create_bucket(string &bucket_name);


        /**
         * check bucket exist or not
         * @param bucket_name
         * @return
         */
        bool bucket_exists(const string &bucket_name);


        /**
         * create object
         * @param bucket_name
         * @param key
         * @param data
         * @return
         */
        std::string put_object(string stream_id, string &bucket_name, string &key, vector<char> &data);


        /**
         * 按时间方式进行命名
         * bucket=ja-streamer-yyyymmdd
         * key=hh/mm/$object_name
         * @param data
         * @return
         */
        std::string put_object_by_time(string stream_id, string &object_name, vector<char> &data);


        /**
         * 按时间方式进行命名
         * key=yyyymmdd/hh/mm/$object_name
         * @param bucket
         * @param object_name
         * @param data
         * @return
         */
        std::string put_object_by_time(string stream_id, string &bucket, string &object_name, vector<char> &data);


        /**
         * 按时间方式进行命名
         *
         * bucket=yyyymmdd
         * key=hh/mm/$uuid
         * @param data
         * @return
         */
        std::string put_object_by_time(string stream_id, vector<char> &data, std::string suffix);

    protected:
        bool end_with(string str, string suffix);

        time_t get_curr_ts() {
            std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds> tpMicro
                = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
            time_t timeStamp2 = tpMicro.time_since_epoch().count();
            return timeStamp2;
        }
    private:
        S3Storage(){};

        static S3Storage *m_Instance;

        string m_endpoint;
        bool m_https;
        string m_access_id;
        string m_access_secret;

        S3Client *s3_client = {NULL};
        Aws::SDKOptions options;
    };

}
#endif //ZLMEDIAKIT_S3STORAGE_H
#endif
