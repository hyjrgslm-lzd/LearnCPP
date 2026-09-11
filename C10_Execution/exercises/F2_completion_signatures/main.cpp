#include <c10/test.hpp>
#include <solution.hpp>

#include <exception>
#include <string>
#include <type_traits>

namespace {
struct to_string_fn {
  template <class... Args> using result = std::string;
};
struct to_void_fn {
  template <class... Args> using result = void;
};

void check_extraction() {
  using namespace c10_f2;
  using sigs = completion_signatures<set_value_t(int), set_value_t(),
                                     set_error_t(std::exception_ptr), set_stopped_t()>;
  if constexpr (std::is_same_v<value_signatures_t<sigs>,
                               type_list<set_value_t(int), set_value_t()>>) {
    c10::require(true, "value channel signatures extracted");
  } else {
    c10::require(false, "value channel signatures extracted");
  }
  if constexpr (std::is_same_v<error_signatures_t<sigs>,
                               type_list<set_error_t(std::exception_ptr)>> &&
                sends_stopped_v<sigs>) {
    c10::require(true, "error and stopped channels extracted");
  } else {
    c10::require(false, "error and stopped channels extracted");
  }
}

void check_then_transform() {
  using namespace c10_f2;
  using input =
      completion_signatures<set_value_t(int), set_error_t(std::exception_ptr), set_stopped_t()>;
  using string_output = then_completion_signatures_t<input, to_string_fn>;
  using void_output = then_completion_signatures_t<input, to_void_fn>;
  if constexpr (std::is_same_v<string_output, completion_signatures<set_value_t(std::string),
                                                                    set_error_t(std::exception_ptr),
                                                                    set_stopped_t()>>) {
    c10::require(true, "then transforms value result type");
  } else {
    c10::require(false, "then transforms value result type");
  }
  if constexpr (std::is_same_v<void_output,
                               completion_signatures<set_value_t(), set_error_t(std::exception_ptr),
                                                     set_stopped_t()>>) {
    c10::require(true, "void value transform emits set_value with no arguments");
  } else {
    c10::require(false, "void value transform emits set_value with no arguments");
  }
}
} // namespace

int main() {
  return c10::test_main([] {
    check_extraction();
    check_then_transform();
  });
}
