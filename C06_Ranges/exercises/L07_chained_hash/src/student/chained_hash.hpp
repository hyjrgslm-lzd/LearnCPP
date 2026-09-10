#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

namespace c06_l07 {

class StringIntMap {
public:
    explicit StringIntMap(std::size_t) {}

    static std::size_t bucket_for(std::string_view, std::size_t) { return 0; }
    static std::string first_key_for_bucket(std::size_t, std::size_t, std::string_view) { return {}; }

    bool put(std::string, int) { return false; }
    std::optional<int> get(std::string_view) const { return std::nullopt; }
    bool erase(std::string_view) { return false; }
    void rehash(std::size_t) {}
    std::size_t size() const { return 0; }
    std::size_t bucket_count() const { return 1; }
    std::size_t bucket_size(std::size_t) const { return 0; }
};

} // namespace c06_l07
