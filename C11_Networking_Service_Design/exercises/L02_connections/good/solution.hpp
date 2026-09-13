#pragma once
#include <cstddef>
#include <list>
#include <memory>
#include <algorithm>
#include <span>
#include <stdexcept>
#include <string_view>
namespace exercise {
class queue {
    struct block { std::unique_ptr<char[]> data; std::size_t size; };
    std::list<block> pending;
    std::size_t allocated = 0, sent = 0;
    bool blocked = false;
public:
    bool enqueue(std::string_view s) {
        if (s.size() > 65536-allocated) return false;
        if (s.empty()) return true;
        auto n = s.size();
        block b{std::unique_ptr<char[]>(new char[n]), n};
        std::copy(s.begin(), s.end(), b.data.get());
        pending.push_back(std::move(b)); allocated += n;
        if (allocated >= 49152) blocked = true;
        return true;
    }
    std::span<const char> front() const { return pending.empty() ? std::span<const char>{} : std::span<const char>{pending.front().data.get(), pending.front().size}.subspan(sent); }
    void consume(std::size_t n) {
        if (!n || n > front().size()) throw std::invalid_argument("invalid send completion");
        sent += n;
        if (sent == pending.front().size) { allocated -= pending.front().size; pending.pop_front(); sent = 0; }
        if (allocated <= 32768) blocked = false;
    }
    std::size_t bytes() const { return allocated; }
    bool paused() const { return blocked; }
    bool empty() const { return pending.empty(); }
};
}
