#include <c10/test.hpp>
#include <solution.hpp>

#include <string>
#include <type_traits>

namespace {
template <class T> struct add_pointer_meta {
  using type = T *;
};
template <class T> struct is_integral_meta : std::is_integral<T> {};

void check_type_list_algorithms() {
  using namespace c10_f1;
  using list = type_list<int, float, int, double, float>;
  if constexpr (std::is_same_v<concat_t<type_list<int>, type_list<float, double>>,
                               type_list<int, float, double>>) {
    c10::require(true, "concat joins lists");
  } else {
    c10::require(false, "concat joins lists");
  }
  if constexpr (std::is_same_v<unique_t<list>, type_list<int, float, double>>) {
    c10::require(true, "unique preserves first occurrence order");
  } else {
    c10::require(false, "unique preserves first occurrence order");
  }
  if constexpr (std::is_same_v<transform_t<type_list<int, float>, add_pointer_meta>,
                               type_list<int *, float *>>) {
    c10::require(true, "transform maps every type");
  } else {
    c10::require(false, "transform maps every type");
  }
  if constexpr (std::is_same_v<filter_t<type_list<int, float, long>, is_integral_meta>,
                               type_list<int, long>>) {
    c10::require(true, "filter keeps matching types");
  } else {
    c10::require(false, "filter keeps matching types");
  }
}

void check_sender_style_composition() {
  using namespace c10_f1;
  using sender_a = type_list<int>;
  using sender_b = type_list<float, std::string>;
  using merged = concat_t<sender_a, sender_b>;
  using normalized = unique_t<concat_t<merged, type_list<int, std::string>>>;
  if constexpr (std::is_same_v<normalized, type_list<int, float, std::string>>) {
    c10::require(true, "type_list composes sender value packs");
  } else {
    c10::require(false, "type_list composes sender value packs");
  }
}
} // namespace

int main() {
  return c10::test_main([] {
    check_type_list_algorithms();
    check_sender_style_composition();
  });
}
