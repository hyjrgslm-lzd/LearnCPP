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
    explicit StringIntMap(std::size_t n) : buckets_(n == 0 ? 1 : n) {}

    static std::size_t bucket_for(std::string_view key, std::size_t n) {
        std::size_t h = 2166136261u;
        for (unsigned char c : key) {
            h = (h ^ c) * 16777619u;
        }
        return h % (n == 0 ? 1 : n);
    }

    static std::string first_key_for_bucket(std::size_t bucket, std::size_t n, std::string_view avoid) {
        for (int i = 0; i != 10000; ++i) {
            auto key = "g" + std::to_string(i);
            if (key != avoid && bucket_for(key, n) == bucket) {
                return key;
            }
        }
        return "fallback";
    }

    bool put(std::string key, int value) {
        auto& b = buckets_[bucket_for(key, buckets_.size())];
        for (auto& e : b) {
            if (e.first == key) {
                e.second = value;
                return false;
            }
        }
        b.push_back({std::move(key), value});
        ++size_;
        return true;
    }

    std::optional<int> get(std::string_view key) const {
        for (const auto& b : buckets_) {
            for (const auto& e : b) {
                if (e.first == key) {
                    return e.second;
                }
            }
        }
        return std::nullopt;
    }

    bool erase(std::string_view key) {
        auto index = bucket_for(key, buckets_.size());
        auto& b = buckets_[index];
        for (std::size_t i = 0; i != b.size(); ++i) {
            if (b[i].first == key) {
                b.erase(b.begin() + static_cast<std::ptrdiff_t>(i));
                --size_;
                return true;
            }
        }
        return false;
    }

    void rehash(std::size_t n) {
        std::vector<std::pair<std::string, int>> entries;
        for (auto& b : buckets_) {
            entries.insert(entries.end(), std::make_move_iterator(b.begin()), std::make_move_iterator(b.end()));
        }
        buckets_.assign(n == 0 ? 1 : n, {});
        size_ = 0;
        for (auto& e : entries) {
            put(std::move(e.first), e.second);
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
