#pragma once

#include "support/memory_model.hpp"

#include <cstddef>
#include <initializer_list>

namespace l05 {

class IntBuffer {
public:
    IntBuffer() = default;
    IntBuffer(std::initializer_list<int> values);
    ~IntBuffer();

    IntBuffer(IntBuffer const& other);
    IntBuffer& operator=(IntBuffer const& other);
    IntBuffer(IntBuffer&& other) noexcept;
    IntBuffer& operator=(IntBuffer&& other) noexcept;

    bool empty() const noexcept;
    std::size_t size() const;
    int get(std::size_t index) const;
    void set(std::size_t index, int value);

private:
    l05_support::Handle handle_{};
};

} // namespace l05
