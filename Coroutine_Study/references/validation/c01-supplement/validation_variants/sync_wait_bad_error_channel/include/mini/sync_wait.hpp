#pragma once
#include "mini/task.hpp"
#include <optional>
#include <tuple>
#include <utility>
namespace mini {
template <typename T>
std::optional<std::tuple<T>> sync_wait(task<T>&& t) {
    t.h_.resume();
    auto& r = t.h_.promise().result_;
    if (r.index() == 2) return std::tuple<T>{T{}};
    return std::tuple<T>{std::move(std::get<1>(r))};
}
inline std::optional<std::tuple<>> sync_wait(task<void>&& t) { t.h_.resume(); return std::tuple<>{}; }
template <typename T, typename Sender>
std::optional<std::tuple<T>> sync_wait(Sender&&) { return std::nullopt; }
} // namespace mini
