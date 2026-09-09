#include "int_buffer.hpp"

#include <utility>

namespace l05 {

IntBuffer::IntBuffer(std::initializer_list<int> values)
    : handle_(l05_support::allocate(values)) {
}

IntBuffer::~IntBuffer() {
    l05_support::release(handle_);
}

IntBuffer::IntBuffer(IntBuffer const& other)
    : handle_(other.empty() ? l05_support::Handle{} : l05_support::clone(other.handle_)) {
}

IntBuffer& IntBuffer::operator=(IntBuffer const& other) {
    if (this == &other) {
        return *this;
    }
    auto replacement = other.empty() ? l05_support::Handle{} : l05_support::clone(other.handle_);
    l05_support::release(handle_);
    handle_ = replacement;
    return *this;
}

IntBuffer::IntBuffer(IntBuffer&& other) noexcept
    : handle_(std::exchange(other.handle_, {})) {
}

IntBuffer& IntBuffer::operator=(IntBuffer&& other) noexcept {
    if (this == &other) {
        return *this;
    }
    l05_support::release(handle_);
    handle_ = std::exchange(other.handle_, {});
    return *this;
}

bool IntBuffer::empty() const noexcept {
    return handle_.id == 0;
}

std::size_t IntBuffer::size() const {
    return empty() ? 0 : l05_support::size(handle_);
}

int IntBuffer::get(std::size_t index) const {
    return l05_support::get(handle_, index);
}

void IntBuffer::set(std::size_t index, int value) {
    l05_support::set(handle_, index, value);
}

} // namespace l05
