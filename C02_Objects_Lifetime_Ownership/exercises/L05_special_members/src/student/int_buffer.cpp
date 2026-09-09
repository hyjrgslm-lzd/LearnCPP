#include "int_buffer.hpp"

#include <stdexcept>

namespace l05 {

IntBuffer::IntBuffer(std::initializer_list<int>) {}
IntBuffer::~IntBuffer() = default;
IntBuffer::IntBuffer(IntBuffer const&) {}
IntBuffer& IntBuffer::operator=(IntBuffer const&) { return *this; }
IntBuffer::IntBuffer(IntBuffer&&) noexcept = default;
IntBuffer& IntBuffer::operator=(IntBuffer&&) noexcept = default;

bool IntBuffer::empty() const noexcept {
    return true;
}

std::size_t IntBuffer::size() const {
    return 0;
}

int IntBuffer::get(std::size_t) const {
    throw std::logic_error("student implementation incomplete");
}

void IntBuffer::set(std::size_t, int) {
    throw std::logic_error("student implementation incomplete");
}

} // namespace l05
