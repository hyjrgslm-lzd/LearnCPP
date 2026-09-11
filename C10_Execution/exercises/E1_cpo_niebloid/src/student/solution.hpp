#pragma once

#include <c10/test.hpp>

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

inline std::string describe(...) {
  throw c10::unfinished("E1 CPO: implement describe customization point object");
}

inline std::string name_of(...) {
  throw c10::unfinished("E1 CPO: implement name_of customization point object");
}

} // namespace c10_e1

inline std::string greet(const c10_e1::Widget &widget) { return "third-party:" + widget.name; }
