#pragma once

#include <span>
#include <stdexcept>

namespace p1 {

template <class T>
class object_buffer {
public:
    object_buffer() = default;
    object_buffer(const object_buffer&) = delete;
    object_buffer& operator=(const object_buffer&) = delete;
    object_buffer(object_buffer&&) noexcept = default;
    object_buffer& operator=(object_buffer&&) noexcept = default;

    [[nodiscard]] std::size_t size() const noexcept { return 0; }
    [[nodiscard]] std::size_t capacity() const noexcept { return 0; }
    [[nodiscard]] std::span<const T> view() const noexcept { return {}; }

    void reserve(std::size_t) {}
    void push_back(T) { throw std::logic_error("student object_buffer not implemented"); }
    void pop_back() { throw std::out_of_range("student object_buffer not implemented"); }
    void clear() noexcept {}
};

} // namespace p1
