#include <object_buffer.hpp>

#include <stdexcept>

namespace {

struct CopyFallback {
    static inline int alive = 0;
    static inline int copies = 0;
    static inline int moves = 0;
    static inline int throw_on_copy = -1;

    int value = 0;

    explicit CopyFallback(int next = 0) : value(next) { ++alive; }

    CopyFallback(const CopyFallback& other) : value(other.value)
    {
        ++copies;
        if (throw_on_copy == copies) {
            throw std::runtime_error("planned copy failure");
        }
        ++alive;
    }

    CopyFallback(CopyFallback&& other) noexcept(false) : value(other.value)
    {
        other.value = -1000;
        ++moves;
        ++alive;
    }

    CopyFallback& operator=(const CopyFallback&) = delete;
    CopyFallback& operator=(CopyFallback&&) = delete;

    ~CopyFallback() noexcept { --alive; }
};

} // namespace

int main()
{
    p1::object_buffer<CopyFallback> buffer;
    buffer.reserve(1);
    buffer.push_back(CopyFallback{1});

    CopyFallback value{2};
    CopyFallback::copies = 0;
    CopyFallback::moves = 0;
    CopyFallback::throw_on_copy = 2;

    try {
        buffer.push_back(static_cast<CopyFallback&&>(value));
    } catch (const std::runtime_error&) {
        return buffer.size() == 1 && buffer.view()[0].value == 1 ? 0 : 2;
    }
    return 3;
}
