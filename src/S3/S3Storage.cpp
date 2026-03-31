//
// Created by 曹旭峰 on 2021/9/5.
//

#if defined(ENABLE_S3_STORAGE)
#include <sstream>
#include "S3Storage.h"
#include "Util/logger.h"
#include "Common/StreamMonitor.h"

using namespace Aws::S3;
using namespace Aws::S3::Model;
using namespace toolkit;

namespace mediakit{

    S3Storage::~S3Storage() {
        if (this->s3_client != nullptr) {
            delete this->s3_client;
        }
        Aws::ShutdownAPI(this->options);
    }


    bool S3Storage::InitS3(string endpoint, string access_id, string access_secret, bool https){

        m_endpoint = endpoint;
        m_access_id = access_id;
        m_access_secret = access_secret;
        m_https = https;

        Aws::InitAPI(this->options);
        Aws::Client::ClientConfiguration cfg;
        cfg.endpointOverride = endpoint;

        cfg.scheme = https ? Aws::Http::Scheme::HTTPS : Aws::Http::Scheme::HTTP;
        cfg.verifySSL = https;

        cfg.httpRequestTimeoutMs = 6 * 1000;
        cfg.requestTimeoutMs = 1 * 1000;
        cfg.region = Aws::String("");

        s3_client = new S3Client(Aws::Auth::AWSCredentials(access_id, access_secret), cfg,
                                 Aws::Client::AWSAuthV4Signer::PayloadSigningPolicy::Never, false);

        return true;
    }

/**
 * create bucket
 * @param bucket_name
 * @return
 */
bool S3Storage::create_bucket(string &bucket_name) {
    //auto start_mills = TimeUtil::now_mills();
    if(!this || this->s3_client == nullptr){
        ErrorL << "S3 Storage is not Ready!!";
        return "";
    }
    InfoL << "creating bucket: " << bucket_name.c_str() <<endl;
    Aws::S3::Model::CreateBucketRequest request;
    request.SetBucket(bucket_name);
    auto result = this->s3_client->CreateBucket(request);
    if (result.IsSuccess()) {
        InfoL << "create bucket success,cost=  ms" << endl; //, TimeUtil::cost_mills(start_mills));
        return true;
    }

    ErrorL << "create bucket failed,msg="<<result.GetError().GetMessage().c_str() << ",cost=%ld ms" <<endl; //, TimeUtil::cost_mills(start_mills));
    return false;
}


/**
 * check bucket exists
 * @param bucket_name
 * @return
 */
bool S3Storage::bucket_exists(const string &bucket_name) {
   if(!this || this->s3_client == nullptr){
       ErrorL << "S3 Storage is not Ready!!";
       return "";
   }
    auto result = s3_client->ListBuckets();
    if (result.IsSuccess()) {
        for (auto bucket: result.GetResult().GetBuckets()) {
            if (bucket.GetName().compare(bucket_name) == 0) {
                return true;
            }
        }
    }

    return false;
}


/**
 * put object
 * @param bucket_name
 * @param key
 * @param data
 * @return
 */
std::string S3Storage::put_object(string stream_id, string &bucket_name, string &key, vector<char> &data) {

    if(!this || this->s3_client == nullptr){
        ErrorL << "S3 Storage is not Ready!!";
        return "";
    }

    if(data.empty()){
        WarnL << "putting object,stream id:"<< stream_id <<"bucket=" << bucket_name.c_str() <<",key=" << key.c_str() << ",size= " << data.size();
        return "";
    }
    time_t start_ts = get_curr_ts();

    PutObjectRequest put_object_request;
    put_object_request.WithBucket(bucket_name.c_str()).WithKey(key.c_str());

    //set request content type for preview in browser
    if (end_with(key, "ts")){
        put_object_request.SetContentType("ts");
    }

    auto object_stream = Aws::MakeShared<Aws::StringStream>("PutObjectInputStream",
                                                            std::stringstream::in | std::stringstream::out |
                                                            std::stringstream::binary);


    object_stream->write(data.data(), data.size());
    put_object_request.SetBody(object_stream);
    object_stream->flush();

    StreamInfo stream_info = { stream_id, PROTOCOL_TTP, ACTION_PUSH, "-"};
    try{
        auto result = this->s3_client->PutObject(put_object_request);
        if (result.IsSuccess()) {
            int write_cost = get_curr_ts() - start_ts;
            InfoL << "put object success, Stream id: "<< stream_id <<"bucket="<< bucket_name.c_str() <<",key="<< key.c_str() <<",size=" << data.size() <<",cost=" << write_cost <<"ms" << endl;
            //写录像成功，把写录像的时间添加到监控
            auto &histogram = StreamMonitor::get().getHistogram(stream_info, HLS_WRITE_COST);
            histogram.Observe((double)write_cost);
            return "/" + bucket_name + key;
        } else {
            ErrorL << "put object failed,bucket="<< bucket_name.c_str() <<",key=" << key.c_str() <<",size="<< data.size() <<
                ",msg="<< result.GetError().GetMessage().c_str();
        }
    }
    catch (const std::exception& e) {
        ErrorL << "S3 putObject catch exception, msg=" << e.what();
    }
    //失败
    auto &counter = StreamMonitor::get().getCounter(stream_info, HLS_WRITE_FAILED_TOTAL);
    counter.Increment();
    return "";
}


    /**
    * 按时间方式进行命名
    * bucket=ja-streamer-yyyymmdd
    * key=hh/mm/$object_name
    * @param data
    * @return
    */
    std::string S3Storage::put_object_by_time(string stream_id, string &object_name, vector<char> &data) {
        //auto date_time = TimeUtil::now_mills();
        std::stringstream bucket_stream, key_stream;

        //bucket: ja-streamer-20220101
        //bucket_stream << "ja-streamer-" << TimeUtil::format(date_time, "%Y%m%d");
        key_stream << "/" << /*TimeUtil::format(date_time, "%H/%m") << "/" << */ object_name;

        auto bucket = bucket_stream.str();
        auto key = key_stream.str();

        bool exists = this->bucket_exists(bucket);
        if (!exists) {
            this->create_bucket(bucket);
        }

        return this->put_object(stream_id, bucket, key, data);

    }


    /**
    * 按时间方式进行命名
    * key=yyyymmdd/hh/mm/$object_name
    * @param bucket
    * @param object_name
    * @param data
    * @return
    */
    std::string S3Storage::put_object_by_time(string stream_id, string &bucket, string &object_name, vector<char> &data) {
        //auto now_mills = TimeUtil::now_mills();
        std::stringstream key_stream;

        key_stream << "/" /*<< TimeUtil::format(now_mills, "%H/%m")*/ << object_name;
        auto key = key_stream.str();
        return this->put_object(stream_id, bucket, key, data);
    }


    /**
     * 按时间方式进行命名
     *
     * bucket=yyyymmdd
     * key=hh/mm/$uuid
     * @param data
     * @return
     */
    std::string S3Storage::put_object_by_time(string stream_id, vector<char> &data, std::string suffix) {
        std::string uuid = Aws::Utils::UUID::RandomUUID();
        auto object_name = uuid + suffix;
        return this->put_object_by_time(stream_id, object_name, data);

    }

    bool S3Storage::end_with(string str, string suffix) {
        int l_suffix = suffix.length();
        int l = str.length();
        if(l == 0 || l_suffix == 0 || l < l_suffix)
            return false;

        for(int i=l_suffix-1,j=l-1; i>=0 && j>=0; i--,j--){
            if(suffix[i] != str[j])
                return false;
        }
        return true;
    }
}



#endif
