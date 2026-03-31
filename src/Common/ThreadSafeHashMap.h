/*
 * Lightweight thread-safe hash map utility copied from ja-media-fs.
 */

#ifndef ZLMEDIAKIT_THREADSAFEHASHMAP_H
#define ZLMEDIAKIT_THREADSAFEHASHMAP_H

#include <mutex>
#include <unordered_map>

template <typename K, typename V>
class ThreadSafeHashMap {
public:
    void insert(const K &key, const V &value) {
        std::lock_guard<std::mutex> lock(_mutex);
        _map[key] = value;
    }

    bool find(const K &key, V &value) const {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _map.find(key);
        if (it == _map.end()) {
            return false;
        }
        value = it->second;
        return true;
    }

    bool erase(const K &key) {
        std::lock_guard<std::mutex> lock(_mutex);
        return _map.erase(key) > 0;
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(_mutex);
        return _map.size();
    }

private:
    mutable std::mutex _mutex;
    std::unordered_map<K, V> _map;
};

#endif // ZLMEDIAKIT_THREADSAFEHASHMAP_H
