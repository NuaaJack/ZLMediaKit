/*
 * Copyright (c) 2016-present The ZLMediaKit project authors. All Rights Reserved.
 *
 * This file is part of ZLMediaKit(https://github.com/ZLMediaKit/ZLMediaKit).
 *
 * Use of this source code is governed by MIT-like license that can be found in the
 * LICENSE file in the root of the source tree. All contributing project authors
 * may be found in the AUTHORS file in the root of the source tree.
 */

#ifndef ZLMEDIAKIT_MULTILINKCONTROL_H
#define ZLMEDIAKIT_MULTILINKCONTROL_H

#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include "Extension/Frame.h"

namespace mediakit {

class MultiMediaSourceMuxer;

class MultiLinkControl : public std::enable_shared_from_this<MultiLinkControl> {
public:
    using Ptr = std::shared_ptr<MultiLinkControl>;

    static Ptr Instance();

    // Parse "..._acs500" to "...", and output priority = 500.
    static bool parseAcsStream(const std::string &origin_stream, std::string &real_stream, int &priority);

    void registerMuxer(const std::shared_ptr<MultiMediaSourceMuxer> &muxer, const std::string &real_stream, int priority);
    void unregisterMuxer(const std::string &real_stream, const std::string &origin_stream);
    void inputFrame(const std::shared_ptr<MultiMediaSourceMuxer> &muxer, const std::string &real_stream, int priority, const Frame::Ptr &frame);

private:
    struct LinkState {
        std::weak_ptr<MultiMediaSourceMuxer> muxer;
        int priority = 0;
        std::deque<uint64_t> video_frame_ms;
        std::deque<uint64_t> all_frame_ms;
    };

    struct GroupState {
        std::shared_ptr<MultiMediaSourceMuxer> master_muxer;
        std::unordered_map<std::string, LinkState> links;
        std::string active_stream;
    };

private:
    static uint64_t nowMs();
    static void trimFrames(std::deque<uint64_t> &frames, uint64_t now_ms);
    static int calcFps(const std::deque<uint64_t> &frames);
    std::string pickBestLink(GroupState &group, uint64_t now_ms);
    void attachChildTracks(const std::shared_ptr<MultiMediaSourceMuxer> &master, const std::shared_ptr<MultiMediaSourceMuxer> &child);

private:
    std::mutex _mtx;
    std::unordered_map<std::string, GroupState> _groups;
};

} // namespace mediakit

#endif // ZLMEDIAKIT_MULTILINKCONTROL_H
