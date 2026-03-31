/*
 * Copyright (c) 2016-present The ZLMediaKit project authors. All Rights Reserved.
 *
 * This file is part of ZLMediaKit(https://github.com/ZLMediaKit/ZLMediaKit).
 *
 * Use of this source code is governed by MIT-like license that can be found in the
 * LICENSE file in the root of the source tree. All contributing project authors
 * may be found in the AUTHORS file in the root of the source tree.
 */

#include "MultiLinkControl.h"
#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstdlib>
#include <limits>
#include "MultiMediaSourceMuxer.h"

using namespace std;

namespace mediakit {

namespace {
constexpr uint64_t kFpsWindowMs = 3000;
constexpr int kPriorityMax = 1000;

int parsePriority(const string &suffix) {
    string digits;
    digits.reserve(suffix.size());
    for (auto ch : suffix) {
        if (std::isdigit(static_cast<unsigned char>(ch))) {
            digits.push_back(ch);
        }
    }
    if (digits.empty()) {
        return 0;
    }
    auto value = atoi(digits.c_str());
    if (value < 0) {
        value = 0;
    } else if (value > kPriorityMax) {
        value = kPriorityMax;
    }
    return value;
}
} // namespace

MultiLinkControl::Ptr MultiLinkControl::Instance() {
    static MultiLinkControl::Ptr s_instance = std::make_shared<MultiLinkControl>();
    return s_instance;
}

bool MultiLinkControl::parseAcsStream(const string &origin_stream, string &real_stream, int &priority) {
    real_stream = origin_stream;
    priority = 0;
    if (origin_stream.empty()) {
        return false;
    }

    auto pos = origin_stream.find_last_of('_');
    if (pos == string::npos || pos + 1 >= origin_stream.size()) {
        return false;
    }

    auto tail = origin_stream.substr(pos + 1);
    string lower = tail;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return std::tolower(c); });
    auto acs_pos = lower.find("acs");
    if (acs_pos == string::npos) {
        return false;
    }

    real_stream = origin_stream.substr(0, pos);
    if (real_stream.empty()) {
        real_stream = origin_stream;
        return false;
    }
    priority = parsePriority(tail.substr(acs_pos + 3));
    return true;
}

uint64_t MultiLinkControl::nowMs() {
    return chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now().time_since_epoch()).count();
}

void MultiLinkControl::trimFrames(deque<uint64_t> &frames, uint64_t now_ms) {
    while (!frames.empty() && now_ms > frames.front() && now_ms - frames.front() > kFpsWindowMs) {
        frames.pop_front();
    }
}

int MultiLinkControl::calcFps(const deque<uint64_t> &frames) {
    if (frames.empty()) {
        return 0;
    }
    return static_cast<int>(frames.size() * 1000 / kFpsWindowMs);
}

string MultiLinkControl::pickBestLink(GroupState &group, uint64_t now_ms) {
    string best_stream;
    int best_fps = -1;
    int best_priority = std::numeric_limits<int>::min();

    for (auto it = group.links.begin(); it != group.links.end();) {
        auto &state = it->second;
        if (state.muxer.expired()) {
            it = group.links.erase(it);
            continue;
        }
        trimFrames(state.video_frame_ms, now_ms);
        trimFrames(state.all_frame_ms, now_ms);
        int fps = calcFps(state.video_frame_ms);
        if (fps <= 0) {
            fps = calcFps(state.all_frame_ms);
        }
        auto priority = state.priority;
        bool better = false;
        if (fps > best_fps) {
            better = true;
        } else if (fps == best_fps && priority > best_priority) {
            better = true;
        } else if (fps == best_fps && priority == best_priority && it->first == group.active_stream) {
            // same score, keep current link to reduce unnecessary switch
            better = true;
        }

        if (better) {
            best_stream = it->first;
            best_fps = fps;
            best_priority = priority;
        }
        ++it;
    }
    return best_stream;
}

void MultiLinkControl::attachChildTracks(const shared_ptr<MultiMediaSourceMuxer> &master, const shared_ptr<MultiMediaSourceMuxer> &child) {
    if (!master || !child) {
        return;
    }
    auto tracks = child->getTracks(true);
    auto listener = child->getDelegate();
    auto attach = [master, tracks, listener]() {
        if (listener) {
            master->setMediaListener(listener);
        }
        if (master->isAllTrackReady() || tracks.empty()) {
            return;
        }
        for (auto &track : tracks) {
            master->addTrack(track);
        }
        master->addTrackCompleted();
    };

    auto poller = master->getOwnerPoller(MediaSource::NullMediaSource());
    if (!poller || poller->isCurrentThread()) {
        attach();
    } else {
        poller->async(std::move(attach));
    }
}

void MultiLinkControl::registerMuxer(const shared_ptr<MultiMediaSourceMuxer> &muxer, const string &real_stream, int priority) {
    if (!muxer || real_stream.empty()) {
        return;
    }
    auto origin_stream = muxer->getMediaTuple().stream;
    shared_ptr<MultiMediaSourceMuxer> master;
    {
        lock_guard<mutex> lck(_mtx);
        auto &group = _groups[real_stream];
        auto &state = group.links[origin_stream];
        state.muxer = muxer;
        state.priority = priority;

        if (!group.master_muxer) {
            auto tuple = muxer->getMediaTuple();
            tuple.stream = real_stream;
            auto option = muxer->getOption();
            option.stream_replace.clear();
            option.modify_stamp = ProtocolOption::kModifyStampOff;
            option.enable_rtmp = true;
            group.master_muxer = std::make_shared<MultiMediaSourceMuxer>(tuple, 0.0f, option);
            InfoL << "ACS create master stream: " << tuple.shortUrl() << " from child: " << origin_stream;
        }
        if (group.active_stream.empty()) {
            group.active_stream = origin_stream;
        }
        master = group.master_muxer;
    }

    attachChildTracks(master, muxer);
}

void MultiLinkControl::unregisterMuxer(const string &real_stream, const string &origin_stream) {
    if (real_stream.empty() || origin_stream.empty()) {
        return;
    }

    unique_lock<mutex> lck(_mtx);
    auto group_it = _groups.find(real_stream);
    if (group_it == _groups.end()) {
        return;
    }
    auto &group = group_it->second;
    group.links.erase(origin_stream);
    if (!group.links.empty()) {
        if (group.active_stream == origin_stream) {
            group.active_stream = pickBestLink(group, nowMs());
        }
        return;
    }

    auto master_to_close = group.master_muxer;
    if (!master_to_close) {
        _groups.erase(group_it);
        return;
    }

    // Close the synthetic master before removing the group so fast reconnects
    // can't create a new master while the old one is still registered.
    auto close_master = [master_to_close]() {
        master_to_close->close(MediaSource::NullMediaSource());
    };
    auto poller = master_to_close->getOwnerPoller(MediaSource::NullMediaSource());
    if (!poller || poller->isCurrentThread()) {
        close_master();
    } else {
        poller->sync(close_master);
    }

    group_it = _groups.find(real_stream);
    if (group_it != _groups.end() && group_it->second.links.empty() && group_it->second.master_muxer == master_to_close) {
        _groups.erase(group_it);
    }
}

void MultiLinkControl::inputFrame(const shared_ptr<MultiMediaSourceMuxer> &muxer, const string &real_stream, int priority, const Frame::Ptr &frame) {
    if (!muxer || !frame || real_stream.empty()) {
        return;
    }
    auto origin_stream = muxer->getMediaTuple().stream;
    auto should_log_missing = frame->getTrackType() != TrackVideo || frame->keyFrame() || frame->configFrame();
    shared_ptr<MultiMediaSourceMuxer> master;
    bool need_forward = false;
    bool switched = false;
    string prev_stream;
    string next_stream;

    {
        lock_guard<mutex> lck(_mtx);
        auto group_it = _groups.find(real_stream);
        if (group_it == _groups.end()) {
            if (should_log_missing) {
                WarnL << "ACS frame ignored because group missing, real stream: " << real_stream
                      << ", child stream: " << origin_stream;
            }
            return;
        }
        auto &group = group_it->second;

        auto &state = group.links[origin_stream];
        state.muxer = muxer;
        state.priority = priority;
        auto now = nowMs();
        state.all_frame_ms.emplace_back(now);
        if (frame->getTrackType() == TrackVideo) {
            state.video_frame_ms.emplace_back(now);
        }
        trimFrames(state.video_frame_ms, now);
        trimFrames(state.all_frame_ms, now);

        auto best = pickBestLink(group, now);
        if (best.empty()) {
            return;
        }
        if (group.active_stream.empty() || group.links.find(group.active_stream) == group.links.end()) {
            group.active_stream = best;
        } else if (best != group.active_stream) {
            bool can_switch = origin_stream == best
                              && (frame->getTrackType() != TrackVideo || frame->keyFrame() || frame->configFrame());
            if (can_switch) {
                prev_stream = group.active_stream;
                group.active_stream = best;
                next_stream = best;
                switched = true;
            }
        }

        need_forward = group.active_stream == origin_stream;
        master = group.master_muxer;
    }

    if (switched) {
        InfoL << "ACS switch stream: " << prev_stream << " -> " << next_stream << " by fps/priority policy";
    }

    if (!need_forward || !master) {
        if (!master && should_log_missing) {
            WarnL << "ACS frame ignored because master missing, real stream: " << real_stream
                  << ", child stream: " << origin_stream;
        }
        return;
    }

    auto cacheable_frame = Frame::getCacheAbleFrame(frame);
    auto forward = [master, cacheable_frame]() {
        master->inputFrame(cacheable_frame);
    };
    auto poller = master->getOwnerPoller(MediaSource::NullMediaSource());
    if (!poller || poller->isCurrentThread()) {
        forward();
    } else {
        poller->async(std::move(forward));
    }
}

} // namespace mediakit
