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
    if (r.index() == 2) {
        try { std::rethrow_exception(std::get<2>(r)); }
        catch (const stopped_exception&) { return std::nullopt; }
    }
    return std::tuple<T>{std::move(std::get<1>(r))};
}
inline std::optional<std::tuple<>> sync_wait(task<void>&& t) {
    t.h_.resume();
    if (t.h_.promise().error_) std::rethrow_exception(t.h_.promise().error_);
    return std::tuple<>{};
}
template <typename T, typename Sender>
std::optional<std::tuple<T>> sync_wait(Sender&&) {
    return std::nullopt;
}
} // namespace mini
