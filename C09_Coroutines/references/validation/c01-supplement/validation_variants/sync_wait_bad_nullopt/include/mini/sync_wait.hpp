#pragma once
#include "mini/task.hpp"
#include <optional>
#include <tuple>
namespace mini {
template <typename T>
std::optional<std::tuple<T>> sync_wait(task<T>&&) { return std::nullopt; }
inline std::optional<std::tuple<>> sync_wait(task<void>&&) { return std::tuple<>{}; }
template <typename T, typename Sender>
std::optional<std::tuple<T>> sync_wait(Sender&&) { return std::nullopt; }
} // namespace mini
