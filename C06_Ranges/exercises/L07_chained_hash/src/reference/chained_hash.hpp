#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace c06_l07 {

class StringIntMap {
public:
    explicit StringIntMap(std::size_t bucket_count) : buckets_(bucket_count == 0 ? 1 : bucket_count) {}

    static std::size_t bucket_for(std::string_view key, std::size_t bucket_count) {
        std::size_t hash = 0;
        for (unsigned char c : key) {
            hash = hash * 131 + c;
        }
        return hash % (bucket_count == 0 ? 1 : bucket_count);
    }

    static std::string first_key_for_bucket(std::size_t bucket, std::size_t bucket_count, std::string_view avoid) {
        for (int i = 0; i != 10000; ++i) {
            std::string key = "k" + std::to_string(i);
            if (key != avoid && bucket_for(key, bucket_count) == bucket) {
                return key;
            }
        }
        return "fallback";
    }

    bool put(std::string key, int value) {
        auto& bucket = buckets_[bucket_for(key, buckets_.size())];
        for (auto& entry : bucket) {
            if (entry.first == key) {
                entry.second = value;
                return false;
            }
        }
        bucket.emplace_back(std::move(key), value);
        ++size_;
        return true;
    }

    std::optional<int> get(std::string_view key) const {
        for (const auto& entry : buckets_[bucket_for(key, buckets_.size())]) {
            if (entry.first == key) {
                return entry.second;
            }
        }
        return std::nullopt;
    }

    bool erase(std::string_view key) {
        auto& bucket = buckets_[bucket_for(key, buckets_.size())];
        for (auto it = bucket.begin(); it != bucket.end(); ++it) {
            if (it->first == key) {
                bucket.erase(it);
                --size_;
                return true;
            }
        }
        return false;
    }

    void rehash(std::size_t bucket_count) {
        std::vector<std::vector<std::pair<std::string, int>>> old = std::move(buckets_);
        buckets_.assign(bucket_count == 0 ? 1 : bucket_count, {});
        size_ = 0;
        for (auto& bucket : old) {
            for (auto& entry : bucket) {
                put(std::move(entry.first), entry.second);
            }
        }
    }

    std::size_t size() const { return size_; }
    std::size_t bucket_count() const { return buckets_.size(); }
    std::size_t bucket_size(std::size_t bucket) const { return buckets_.at(bucket).size(); }

private:
    std::vector<std::vector<std::pair<std::string, int>>> buckets_;
    std::size_t size_{};
};

} // namespace c06_l07
