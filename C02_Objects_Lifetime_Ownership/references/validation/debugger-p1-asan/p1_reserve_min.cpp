#include <object_buffer.hpp>

#include <stdexcept>

namespace {

struct CopyFallback {
    static inline int copies = 0;
    static inline int throw_on_copy = -1;

    int value = 0;

    explicit CopyFallback(int next = 0) : value(next) {}

    CopyFallback(const CopyFallback& other) : value(other.value)
    {
        ++copies;
        if (throw_on_copy == copies) {
            throw std::runtime_error("planned copy failure");
        }
    }

    CopyFallback(CopyFallback&& other) noexcept(false) : value(other.value)
    {
        other.value = -1000;
    }

    CopyFallback& operator=(const CopyFallback&) = delete;
    CopyFallback& operator=(CopyFallback&&) = delete;
};

} // namespace

int main()
{
    p1::object_buffer<CopyFallback> buffer;
    buffer.reserve(2);
    buffer.push_back(CopyFallback{1});
    buffer.push_back(CopyFallback{2});

    CopyFallback::copies = 0;
    CopyFallback::throw_on_copy = 2;

    try {
        buffer.reserve(8);
    } catch (const std::runtime_error&) {
        return buffer.size() == 2 && buffer.view()[0].value == 1 && buffer.view()[1].value == 2 ? 0 : 2;
    }
    return 3;
}
