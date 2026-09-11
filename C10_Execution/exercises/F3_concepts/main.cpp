#include <c10/test.hpp>
#include <solution.hpp>

#include <exception>
#include <string>

namespace {
void check_sender_receiver_concepts() {
  using namespace c10_f3;
  c10::require(my_sender<valid_sender>, "valid sender satisfies syntax concept");
  c10::require(!my_sender<invalid_sender>, "sender concept rejects missing signatures");
  c10::require(my_receiver<valid_receiver>, "valid receiver satisfies syntax concept");
  c10::require(!my_receiver<missing_receiver_tag>, "receiver concept rejects missing tag");
}

void check_receiver_of_and_sender_to() {
  using namespace c10_f3;
  using sigs =
      completion_signatures<set_value_t(int), set_error_t(std::exception_ptr), set_stopped_t()>;
  c10::require(my_receiver_of<valid_receiver, sigs>,
               "receiver_of accepts all declared completions");
  c10::require(!my_receiver_of<partial_receiver, sigs>, "receiver_of rejects semantic mismatch");
  c10::require(my_sender_to<valid_sender, valid_receiver>,
               "sender_to connects sender signatures to receiver");
  c10::require(!my_sender_to<valid_sender, partial_receiver>,
               "sender_to rejects receiver that cannot accept value");
}
} // namespace

int main() {
  return c10::test_main([] {
    check_sender_receiver_concepts();
    check_receiver_of_and_sender_to();
  });
}
