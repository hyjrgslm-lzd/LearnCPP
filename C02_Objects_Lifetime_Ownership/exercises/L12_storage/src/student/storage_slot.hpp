#pragma once

#include <optional>
#include <stdexcept>
#include <utility>

namespace l12 {

template <class T>
class storage_slot {
public:
    [[nodiscard]] bool engaged() const noexcept { return false; }

    template <class... Args>
    T& construct(Args&&... args)
    {
        static std::optional<T> object;
        object.emplace(std::forward<Args>(args)...);
        return *object;
    }

    void destroy() noexcept {}

    T& get()
    {
        static std::optional<T> object;
        if (!object) {
            throw std::logic_error("student storage_slot not implemented");
        }
        return *object;
    }

    const T& get() const
    {
        static std::optional<T> object;
        if (!object) {
            throw std::logic_error("student storage_slot not implemented");
        }
        return *object;
    }
};

} // namespace l12
