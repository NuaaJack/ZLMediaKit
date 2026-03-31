//
// Created by admin on 2022/8/3.
//

#ifndef ZLMEDIAKIT_STREAMMONITOR_H
#define ZLMEDIAKIT_STREAMMONITOR_H

#include <string>
#include <map>
#include <set>
#include <memory>
#include <mutex>
#include "Common/macros.h"
#include "prometheus/client_metric.h"
#include "prometheus/counter.h"
#include "prometheus/exposer.h"
#include "prometheus/family.h"
#include "prometheus/registry.h"
#include <prometheus/summary.h>
#include <prometheus/histogram.h>
#include "Common/ThreadSafeHashMap.h"
#include <srt/srt.h>

using namespace std;


namespace mediakit {
    //内部服务Id在监控中区分流媒体
    static const string METRIC_INTERNAL_SERVER_ID = "internal_server_id";
    static const string METRIC_CREATED = "created";
    static const string METRIC_CLOSED = "closed";
    //编码目标码率,单位 kbps
    static const string METRIC_BITRATE_TARGET = "bitrate_target";
    //视频实际码率,单位 kbps
    static const string METRIC_BITRATE_CURRENT = "bitrate_current";
    //编码器最大码率,单位 kbps
    static const string METRIC_BITRATE_TARGET_MAX = "target_max";
    //编码类型
    static const string METRIC_CODEC = "codec";
    static const string METRIC_VIDEO_TRACK_READY = "ready";
    //发送端的连接状态
    static const string METRIC_SENDER_STATUS = "snd_conn_status";
    static const string METRIC_WIDTH = "width";
    static const string METRIC_HEIGHT = "height";
    static const string METRIC_FPS = "fps";
    static const string METRIC_PKT_SEND = "pkt_send";
    static const string METRIC_PKT_SEND_LOSS = "pkt_send_loss";
    static const string METRIC_RTT = "rtt";
    static const string METRIC_BANDWIDTH = "bandwidth";
    static const string METRIC_LATENCY = "latency";
    static const string METRIC_SEND_RATE = "send_rate";
    static const string METRIC_SEND_BUFF = "send_buf";
    static const string METRIC_DROP_GOP_CNT = "gop_drop";
    static const string METRIC_SUPER_COST = "super_cost";
    static const string METRIC_SUPER_QUEUE = "super_queue";
    static const string METRIC_RENDER_FPS = "render_fps";
    static const string METRIC_RENDER_DELAY = "render_delay";
    static const string METRIC_INPUT_FRAME_COST = "input_frame_cost";
    static const string METRIC_TSBPD_WAIT_TIME = "tsbpd_wait_time";
    static const string METRIC_FRAME_INTERVAL = "frame_interval";
    static const string METRIC_FRAME_LATENCY = "frame_latency";
    static const string METRIC_FRAME_DROP_TOTAL = "frame_drop_total";
    static const string METRIC_FRAME_READ_TOTAL = "frame_read_total";
    static const string METRIC_PKT_ARRIVAL_TOTAL = "pkt_arrival_total";
    static const string METRIC_FRAME_ARRIVAL_COST = "frame_arrival_cost";
    static const string METRIC_PUSH_MODE = "push_mode";
    static const string METRIC_FRAME_FREEZE_TOTAL = "frame_freeze_total";
    static const string METRIC_MSG_READ_TOTAL = "msg_read_total";
    static const string METRIC_CONNECTION_STATUS = "connection_status";
    static const string METRIC_RECV_BUFFER_SIZE = "recv_buffer_size";
    static const string METRIC_FRAME_BUFFER_SIZE = "frame_buffer_size";
    static const string METRIC_TRANSCODE_ENCODE_QUEUE_SIZE = "trans_encode_queue_size";
    static const string METRIC_TRANSCODE_ENCODE_COST = "trans_encode_cost";
    static const string METRIC_TRANSCODE_DECODE_COST = "trans_decode_cost";
    static const string METRIC_FRAME_PLAY_TOTAL = "frame_play_total";
    static const string METRIC_200MS_FREEZE_RATE = "freeze_rate_200ms";
    static const string METRIC_600MS_FREEZE_RATE = "freeze_rate_600ms";
    static const string HLS_WRITE_COST = "hls_write_cost";
    static const string HLS_WRITE_FAILED_TOTAL = "hls_write_failed_total";

    static const string GB_SHARED_IS_UDP = "gb_shared_is_udp";
    static const string GB_SHARED_BYTES = "gb_shared_bytes";

    static const string PROTOCOL_FLV = "flv";
    static const string PROTOCOL_TTP = "ttp";
    static const string PROTOCOL_WEBRTC = "webrtc";
    static const string PROTOCOL_RTMP = "rtmp";
    static const string PROTOCOL_RTSP = "rtsp";
    static const string PROTOCOL_HLS = "hls";

    static const string ACTION_PUSH = "push";
    static const string ACTION_PLAY = "play";

    //播放器指标采集

    static const string METRIC_PLAY_DECODER_TYPE = "decoder_type";
    static const string METRIC_PLAY_FPS = "fps";
    static const string METRIC_PLAY_BITRATE = "bitrate";
    //待解码帧：未送入解码器
    static const string METRIC_PLAY_DEMUX_BUFFER = "demux_buffer";

    static const string METRIC_PLAY_DECODER_MAX_INTERVAL = "decoder_max_interval";
    //解码队列：送入解码器、未完成解码的队列长度
    static const string METRIC_PLAY_DECODER_QUEUE_SIZE = "decoder_queue_size";
    //解码完成，等待渲染队列长度
    static const string METRIC_PLAY_RENDER_QUEUE_SIZE = "render_queue_size";
    //渲染丢帧数量
    static const string METRIC_PLAY_RENDER_DROP_FRAMES = "render_drop_frames";


    static const string METRIC_PLAY_CLIENT_CPU_CORES = "client_cpu_cores";
    static const string METRIC_PLAY_CLIENT_MEMORY = "client_memory";
    static const string METRIC_PLAY_CLIENT_USER_AGENT = "client_user_agent";
    static const string METRIC_PLAY_CLIENT_GPU_INFO = "client_gpu_info";
    static const string METRIC_PLAY_CLIENT_SUPPORT_DECODERS = "client_support_decoders";


    class StreamInfo {
    public:
        //视频流ID
        string stream_id;

        //视频流协议
        string protocol;

        //动态：推流、播放
        string action;

        //客户端ID,action=PUSH 可为空
        string client_id;

        std::map<std::string, std::string> labels;

        StreamInfo();
        StreamInfo(const string &streamId, const string &aProtocol, const string &action, const string &clientId,
                   const std::map<std::string, std::string> &labels = std::map<std::string, std::string>());

        StreamInfo(const StreamInfo &other);


        const std::string toString() const {
            string str = stream_id + "_"
                         + protocol + "_"
                         + action;

            if (action == ACTION_PLAY) {
                str += client_id;
            }
            return str;
        }
    };


    class Metric {
    public:
        std::string type;
        double value;

        Metric(const std::string &type, double value);

        Metric(const Metric &other);
    };

    class Listener {
    public:
        function<void(const StreamInfo &, const Metric)> callback;
    };


    class StreamMonitor {
    public:

        static void setPort(int port) {
            StreamMonitor::port = port;
        }

        static StreamMonitor &get() {
            static StreamMonitor instance;
            return instance;
        }

        void addStreamInfo(SRTSOCKET socket, const StreamInfo &streamInfo);

        bool getStreamInfo(SRTSOCKET socket, StreamInfo &streamInfo);

        void removeStreamInfo(SRTSOCKET socket);

        void collect(const StreamInfo &streamInfo, const std::vector<Metric> &metrics);

        prometheus::Gauge &getGauge(const StreamInfo &streamInfo, const std::string &name);

        prometheus::Counter &getCounter(const StreamInfo &streamInfo, const std::string &name);

        prometheus::Summary &getSummary(const StreamInfo &streamInfo, const std::string &name);

        prometheus::Histogram &getHistogram(const StreamInfo &streamInfo, const std::string &name);

        void removeSummary(const StreamInfo &streamInfo);

        void removeCounter(const StreamInfo &streamInfo);

        void removeGauge(const StreamInfo &streamInfo);

        void removeHistogram(const StreamInfo &streamInfo);

        void removeMetric(const StreamInfo &streamInfo);
        
        // 只根据stream_id、protocol、action匹配删除，忽略client_id
        void removeMetricByStreamId(const string &stream_id, const string &protocol, const string &action);


    private:
        static int port;
    private:
        // Prometheus family/container operations are not thread-safe.
        mutable std::recursive_mutex metric_mutex;
        //key=name, val=metric
        map<string, prometheus::Family < prometheus::Gauge> *>
        gauge_family_map;
        map<string, prometheus::Family < prometheus::Counter> *>
        counter_family_map;
        map<string, prometheus::Family < prometheus::Summary> *>
        summary_family_map;
        map<string, prometheus::Family < prometheus::Histogram> *>
        histogram_family_map;
        std::shared_ptr<prometheus::Exposer> exposer;
        std::shared_ptr<prometheus::Registry> registry;
        ThreadSafeHashMap<SRTSOCKET, StreamInfo> stream_info_map;


        StreamMonitor() {
            bool isStartPrometheusPort = false;
            while (!isStartPrometheusPort) {
                isStartPrometheusPort = true;
                InfoL << "Starting Prometheus port=" << port;
                string bind_address = "0.0.0.0:" + to_string(port);
                try {
                    this->exposer = std::shared_ptr<prometheus::Exposer>(new prometheus::Exposer(bind_address));
                    this->registry = std::make_shared<prometheus::Registry>();
                    // ask the exposer to scrape the registry on incoming HTTP requests
                    exposer->RegisterCollectable(registry, "/metrics");
                    InfoL << "StreamMonitor Init Success!!";
                } catch (exception &e) {
                    isStartPrometheusPort = false;
                    WarnL << "Current Prometheus port " << port++ << " is unavailable";
                }
            }
        }
    };
}//namespace mediakit
#endif //ZLMEDIAKIT_STREAMMONITOR_H
