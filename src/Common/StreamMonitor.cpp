//
// Created by admin on 2022/8/3.
//

#include <prometheus/counter.h>
#include "StreamMonitor.h"
#include <algorithm>
#include <sstream>



namespace mediakit {

    int StreamMonitor::port = 0;


    static prometheus::Labels create_label(const StreamInfo &streamInfo){
        prometheus::Labels labels = {{"streamId", streamInfo.stream_id},
                                     {"protocol", streamInfo.protocol},
                                     {"action",   streamInfo.action},
                                     {"clientId", streamInfo.client_id}
        };

        if (streamInfo.labels.size() > 0) {
            std::for_each(streamInfo.labels.begin(), streamInfo.labels.end(), [&labels](const std::pair<std::string, std::string> &entry) {
                labels[entry.first] = entry.second;
            });
        }
        return labels;
    }

    prometheus::Gauge &StreamMonitor::getGauge(const StreamInfo &streamInfo, const string &name) {
        lock_guard<recursive_mutex> lck(metric_mutex);
        auto labels = create_label(streamInfo);

        auto it = gauge_family_map.find(name);
        if (it != gauge_family_map.end()) {
            return it->second->Add(labels);
        }else{
            auto &gauge_family = prometheus::BuildGauge().Name(name).Register(*registry);
            gauge_family_map[name] = &gauge_family;
            return gauge_family.Add(labels);
        }
    }



    prometheus::Counter &StreamMonitor::getCounter(const StreamInfo &streamInfo, const string &name) {
        lock_guard<recursive_mutex> lck(metric_mutex);
        auto labels = create_label(streamInfo);
        auto it = counter_family_map.find(name);
        if(it != counter_family_map.end()) {
            return it->second->Add(labels);
        }else{
            auto &counter = prometheus::BuildCounter()
                    .Name(name)
                    .Help("流媒体流状态")
                    .Register(*registry);
            counter_family_map[name] = &counter;
            return counter.Add(labels);
        }
    }

    prometheus::Summary &StreamMonitor::getSummary(const StreamInfo &streamInfo, const string &name) {
        lock_guard<recursive_mutex> lck(metric_mutex);
        auto labels = create_label(streamInfo);
        auto quantiles = prometheus::Summary::Quantiles{};
        auto found_iter = summary_family_map.find(name);
        if (found_iter != summary_family_map.end()) {
            return found_iter->second->Add(labels, quantiles);
        }else{
            auto &summary = prometheus::BuildSummary().Name(name).Help("summary").Register(*registry);
            summary_family_map[name] = &summary;
            return summary.Add(labels, quantiles);
        }
    }


    prometheus::Histogram &StreamMonitor::getHistogram(const StreamInfo &streamInfo, const std::string &name) {
        lock_guard<recursive_mutex> lck(metric_mutex);
        auto labels = create_label(streamInfo);
        auto found_iter = histogram_family_map.find(name);
        //只有一个桶
        prometheus::Histogram::BucketBoundaries bucket = {std::numeric_limits<float>::max()};
        if (found_iter != histogram_family_map.end()) {
            return found_iter->second->Add(labels, bucket);
        }else{
            auto &histogram = prometheus::BuildHistogram().Name(name).Help("histogram").Register(*registry);
            histogram_family_map[name] = &histogram;
            return histogram.Add(labels, bucket);
        }
    }

    void StreamMonitor::removeHistogram(const StreamInfo &streamInfo) {
        lock_guard<recursive_mutex> lck(metric_mutex);
        auto labels = create_label(streamInfo);

        for (auto &pair: histogram_family_map) {
            auto &name = pair.first;
            auto &histogram_family = pair.second;
            if (histogram_family == nullptr) continue;
            try {
                if (histogram_family->Has(labels)) {
                    prometheus::Histogram::BucketBoundaries bucket = {std::numeric_limits<float>::max()};
                    auto &histogram = histogram_family->Add(labels, bucket);
                    histogram_family->Remove(&histogram);
                }
            } catch (const std::exception& e) {
                WarnL << "removeHistogram exception name=" << name << ": " << e.what();
            } catch (...) {
                WarnL << "removeHistogram unknown exception name=" << name;
            }
        }

    }
    void StreamMonitor::removeSummary(const StreamInfo &streamInfo) {
        lock_guard<recursive_mutex> lck(metric_mutex);
        auto labels = create_label(streamInfo);
        for (auto &pair: summary_family_map) {
            auto &name = pair.first;
            auto &summary_family = pair.second;
            if (summary_family == nullptr) continue;
            try {
                if (summary_family->Has(labels)) {
                    auto quantiles = prometheus::Summary::Quantiles{};
                    auto &summary = summary_family->Add(labels,quantiles);
                    summary_family->Remove(&summary);
                }
            } catch (const std::exception& e) {
                WarnL << "removeSummary exception name=" << name << ": " << e.what();
            } catch (...) {
                WarnL << "removeSummary unknown exception name=" << name;
            }
        }
    }

    void StreamMonitor::removeCounter(const StreamInfo &streamInfo) {
        lock_guard<recursive_mutex> lck(metric_mutex);
        auto labels = create_label(streamInfo);
        for (auto &pair: counter_family_map) {
            auto &name = pair.first;
            auto &counter_family = pair.second;
            if (counter_family == nullptr) continue;
            try {
                if (counter_family->Has(labels)) {
                    auto &counter = counter_family->Add(labels);
                    counter_family->Remove(&counter);
                }
            } catch (const std::exception& e) {
                WarnL << "removeCounter exception name=" << name << ": " << e.what();
            } catch (...) {
                WarnL << "removeCounter unknown exception name=" << name;
            }
        }
    }

    static string labelStr(prometheus::Labels &labels){
        std::stringstream  ss;
        for (auto &entry: labels) {
            ss << entry.first << "=" << entry.second << ",";
        }
        return ss.str();
    }

    void StreamMonitor::removeGauge(const StreamInfo &streamInfo) {
        lock_guard<recursive_mutex> lck(metric_mutex);
        auto labels = create_label(streamInfo);
        for (auto &pair: gauge_family_map) {
            auto &name = pair.first;
            auto &gauge_family = pair.second;
            if (gauge_family == nullptr) continue;
            try {
                if (gauge_family->Has(labels)) {
                    auto &gauge = gauge_family->Add(labels);
                    InfoL << "remove gauge " << name;
                    gauge_family->Remove(&gauge);
                }
            } catch (const std::exception& e) {
                WarnL << "removeGauge exception name=" << name << ": " << e.what();
            } catch (...) {
                WarnL << "removeGauge unknown exception name=" << name;
            }
        }
    }

    void StreamMonitor::removeMetric(const StreamInfo &streamInfo) {
        lock_guard<recursive_mutex> lck(metric_mutex);
        removeGauge(streamInfo);
        removeCounter(streamInfo);
        removeSummary(streamInfo);
        removeHistogram(streamInfo);
    }

    void StreamMonitor::removeMetricByStreamId(const string &stream_id, const string &protocol, const string &action) {
        // 遍历所有gauge family，查找匹配stream_id、protocol、action的指标并删除
        lock_guard<recursive_mutex> lck(metric_mutex);
        for (auto &pair: gauge_family_map) {
            auto &name = pair.first;
            auto &gauge_family = pair.second;
            // 尝试常见的client_id值
            vector<string> client_ids = {"-", ""};
            for (const auto& client_id : client_ids) {
                prometheus::Labels labels = {{"streamId", stream_id},
                                             {"protocol", protocol},
                                             {"action",   action},
                                             {"clientId", client_id}};
                if (gauge_family->Has(labels)) {
                    auto &gauge = gauge_family->Add(labels);
                    InfoL << "removeMetricByStreamId: remove gauge " << name << " streamId=" << stream_id;
                    gauge_family->Remove(&gauge);
                }
            }
        }
        // 遍历所有counter family
        for (auto &pair: counter_family_map) {
            auto &counter_family = pair.second;
            vector<string> client_ids = {"-", ""};
            for (const auto& client_id : client_ids) {
                prometheus::Labels labels = {{"streamId", stream_id},
                                             {"protocol", protocol},
                                             {"action",   action},
                                             {"clientId", client_id}};
                if (counter_family->Has(labels)) {
                    auto &counter = counter_family->Add(labels);
                    counter_family->Remove(&counter);
                }
            }
        }
        // 遍历所有summary family
        for (auto &pair: summary_family_map) {
            auto &summary_family = pair.second;
            vector<string> client_ids = {"-", ""};
            for (const auto& client_id : client_ids) {
                prometheus::Labels labels = {{"streamId", stream_id},
                                             {"protocol", protocol},
                                             {"action",   action},
                                             {"clientId", client_id}};
                if (summary_family->Has(labels)) {
                    auto quantiles = prometheus::Summary::Quantiles{};
                    auto &summary = summary_family->Add(labels, quantiles);
                    summary_family->Remove(&summary);
                }
            }
        }
        // 遍历所有histogram family
        for (auto &pair: histogram_family_map) {
            auto &histogram_family = pair.second;
            vector<string> client_ids = {"-", ""};
            for (const auto& client_id : client_ids) {
                prometheus::Labels labels = {{"streamId", stream_id},
                                             {"protocol", protocol},
                                             {"action",   action},
                                             {"clientId", client_id}};
                if (histogram_family->Has(labels)) {
                    prometheus::Histogram::BucketBoundaries bucket = {std::numeric_limits<float>::max()};
                    auto &histogram = histogram_family->Add(labels, bucket);
                    histogram_family->Remove(&histogram);
                }
            }
        }
    }


    void StreamMonitor::collect(const StreamInfo &streamInfo, const std::vector<Metric> &metrics) {
        lock_guard<recursive_mutex> lck(metric_mutex);
        for (auto &metric: metrics) {
            auto &gauge = getGauge(streamInfo, metric.type);
            gauge.Set(metric.value);
        }
    }


    void StreamMonitor::addStreamInfo(SRTSOCKET socket, const mediakit::StreamInfo &streamInfo) {
        stream_info_map.insert(socket, streamInfo);
    }

    bool StreamMonitor::getStreamInfo(SRTSOCKET socket, mediakit::StreamInfo &streamInfo) {
        return stream_info_map.find(socket, streamInfo);
    }

    void StreamMonitor::removeStreamInfo(SRTSOCKET socket) {
        stream_info_map.erase(socket);
    }



    StreamInfo::StreamInfo() {

    }
    StreamInfo::StreamInfo(const string &streamId, const string &aProtocol, const string &action,
                           const string &clientId,const std::map<std::string,std::string>& labels) : stream_id(streamId), protocol(aProtocol), action(action),
                                                     client_id(clientId),labels(labels) {}
    StreamInfo::StreamInfo(const StreamInfo &other) {
        this->stream_id = other.stream_id;
        this->protocol = other.protocol;
        this->action = other.action;
        this->client_id = other.client_id;
        this->labels = other.labels;
    }

    Metric::Metric(const string &type, double value) : type(type), value(value) {}

    Metric::Metric(const Metric &other) {
        this->type = other.type;
        this->value = other.value;
    }
}
