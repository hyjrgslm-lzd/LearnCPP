#include <c10/test.hpp>
#include <solution.hpp>

#include <string>

namespace {

void check_raw_adl_is_only_a_baseline() {
  c10_e1::Cat cat{"cat"};
  c10_e1::Widget widget{"widget"};

  c10::require(greet(cat) == "adl:cat", "raw ADL finds the owned type customization");
  c10::require(greet(widget) == "third-party:widget",
               "raw ADL can be hijacked by associated namespaces");
}

void check_cpo_dispatch_order() {
  c10_e1::Cat cat{"cat"};
  c10_e1::Dog dog{"dog"};
  c10_e1::Widget widget{"widget"};
  c10_e1::Plain plain{"plain"};

  c10::require(c10_e1::describe(cat) == "member:cat", "member customization has first priority");
  c10::require(c10_e1::describe(dog) == "adl-impl:dog",
               "CPO can use a separate ADL implementation hook");
  c10::require(c10_e1::describe(widget) == "member:widget",
               "CPO call object does not invoke raw greet by ADL");
  c10::require(c10_e1::describe(plain) == "default:plain", "CPO keeps an explicit default path");
}

void check_name_result_cpo() {
  c10_e1::Cat cat{"nora"};
  c10_e1::Dog dog{"rex"};
  c10_e1::Plain plain{"blob"};

  c10::require(c10_e1::name_of(cat) == "nora", "name_of supports member path");
  c10::require(c10_e1::name_of(dog) == "dog:rex", "name_of supports ADL implementation path");
  c10::require(c10_e1::name_of(plain) == "blob", "name_of supports fallback field path");
}

} // namespace

int main() {
  return c10::test_main([] {
    check_raw_adl_is_only_a_baseline();
    check_cpo_dispatch_order();
    check_name_result_cpo();
  });
}
