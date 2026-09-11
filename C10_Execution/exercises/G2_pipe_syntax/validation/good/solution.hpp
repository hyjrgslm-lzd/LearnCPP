#pragma once

#include <stdexec/execution.hpp>

#include <type_traits>
#include <utility>

namespace c10_g2 {

namespace ex = stdexec;

template <class F> struct then_closure : ex::sender_adaptor_closure<then_closure<F>> {
  F function;

  explicit then_closure(F f) : function(std::move(f)) {}

  template <ex::sender Sender> auto operator()(Sender &&sender) && {
    return ex::then(std::forward<Sender>(sender), std::move(function));
  }

  template <ex::sender Sender> auto operator()(Sender &&sender) const & {
    return ex::then(std::forward<Sender>(sender), function);
  }
};

template <class F> auto then(F f) { return then_closure<std::remove_cvref_t<F>>{std::move(f)}; }

} // namespace c10_g2
