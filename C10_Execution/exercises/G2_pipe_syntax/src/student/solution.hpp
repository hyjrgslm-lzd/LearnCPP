#pragma once

#include <c10/test.hpp>
#include <stdexec/execution.hpp>

#include <concepts>
#include <functional>
#include <type_traits>
#include <utility>

namespace c10_g2 {

namespace ex = stdexec;

template <class T> struct pipe_closure_base {};

template <class T>
concept pipe_closure =
    std::derived_from<std::remove_cvref_t<T>, pipe_closure_base<std::remove_cvref_t<T>>>;

template <class F> struct unfinished_fn {
  F f_;

  template <class... Args> auto operator()(Args &&...) -> std::invoke_result_t<F, Args...> {
    throw c10::unfinished("G2 pipe syntax: implement closure piping");
    if constexpr (!std::is_void_v<std::invoke_result_t<F, Args...>>) {
      return std::invoke(f_, std::forward<Args>(Args{})...);
    }
  }
};

template <class F> struct then_closure : pipe_closure_base<then_closure<F>> {
  F f_;
  explicit then_closure(F f) : f_(std::move(f)) {}

  template <ex::sender Sender> auto operator()(Sender &&sender) && {
    return ex::then(std::forward<Sender>(sender), unfinished_fn<F>{std::move(f_)});
  }
};

template <class Left, class Right>
struct composed_closure : pipe_closure_base<composed_closure<Left, Right>> {
  Left left_;
  Right right_;
  composed_closure(Left left, Right right) : left_(std::move(left)), right_(std::move(right)) {}

  template <ex::sender Sender> auto operator()(Sender &&sender) && {
    return std::move(right_)(std::move(left_)(std::forward<Sender>(sender)));
  }
};

template <ex::sender Sender, pipe_closure Closure>
auto operator|(Sender &&sender, Closure &&closure) {
  return std::forward<Closure>(closure)(std::forward<Sender>(sender));
}

template <pipe_closure Left, pipe_closure Right> auto operator|(Left &&left, Right &&right) {
  return composed_closure<std::remove_cvref_t<Left>, std::remove_cvref_t<Right>>{
      std::forward<Left>(left), std::forward<Right>(right)};
}

template <class F> auto then(F f) { return then_closure<std::remove_cvref_t<F>>{std::move(f)}; }

} // namespace c10_g2
