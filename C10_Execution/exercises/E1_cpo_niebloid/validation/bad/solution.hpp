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

struct unsafe_describe_t {
  template <class T> std::string operator()(const T &value) const {
    if constexpr (requires { greet(value); }) {
      return greet(value);
    } else if constexpr (requires { greet_impl(value); }) {
      return greet_impl(value);
    } else {
      return "default:" + value.name;
    }
  }
};

struct name_t {
  template <class T> std::string operator()(const T &value) const {
    if constexpr (requires { value.member_name(); }) {
      return value.member_name();
    } else if constexpr (requires { name_of_impl(value); }) {
      return name_of_impl(value);
    } else {
      return value.name;
    }
  }
};

inline constexpr unsafe_describe_t describe{};
inline constexpr name_t name_of{};

} // namespace c10_e1

inline std::string greet(const c10_e1::Widget &widget) { return "third-party:" + widget.name; }
