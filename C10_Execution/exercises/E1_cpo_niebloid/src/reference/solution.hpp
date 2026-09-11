#pragma once

#include <string>

namespace c10_e1 {

struct Cat {
  std::string name;
  std::string describe() const { return "member:" + name; }
  std::string member_name() const { return name; }
};

struct Dog {
  std::string name;
};

struct Widget : Cat {};

struct Plain {
  std::string name;
};

inline std::string greet(const Cat &cat) { return "adl:" + cat.name; }

inline std::string greet_impl(const Dog &dog) { return "adl-impl:" + dog.name; }

inline std::string name_of_impl(const Dog &dog) { return "dog:" + dog.name; }

namespace detail {
void greet_impl();
void name_of_impl();

struct describe_t {
  template <class T> std::string operator()(const T &value) const {
    if constexpr (requires { value.describe(); }) {
      return value.describe();
    } else if constexpr (requires { greet_impl(value); }) {
      return greet_impl(value);
    } else if constexpr (requires { value.name; }) {
      return "default:" + value.name;
    } else {
      static_assert(sizeof(T) == 0, "describe requires member, greet_impl, or name");
    }
  }
};

struct name_of_t {
  template <class T> std::string operator()(const T &value) const {
    if constexpr (requires { value.member_name(); }) {
      return value.member_name();
    } else if constexpr (requires { name_of_impl(value); }) {
      return name_of_impl(value);
    } else if constexpr (requires { value.name; }) {
      return value.name;
    } else {
      static_assert(sizeof(T) == 0, "name_of requires member, name_of_impl, or name");
    }
  }
};

} // namespace detail

inline constexpr detail::describe_t describe{};
inline constexpr detail::name_of_t name_of{};

} // namespace c10_e1

inline std::string greet(const c10_e1::Widget &widget) { return "third-party:" + widget.name; }
