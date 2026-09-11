#include <c10/test.hpp>
#include <stdexec/execution.hpp>

#include <atomic>
#include <array>
#include <iostream>
#include <numeric>
#include <utility>

namespace ex = stdexec;

namespace {

std::atomic<int> domain_lowerings{0};

struct square_domain {
  template <class Sender, class Env>
  auto transform_sender(ex::set_value_t, Sender &&sender, Env const &) const {
    ++domain_lowerings;
    auto value = static_cast<Sender &&>(sender).value;
    return ex::just(value * value);
  }
};

struct square_env {
  auto query(ex::get_completion_domain_t<ex::set_value_t>) const noexcept {
    return square_domain{};
  }
};

struct square_sender {
  using sender_concept = ex::sender_tag;
  int value;

  auto get_env() const noexcept { return square_env{}; }

  template <class Self, class... Env> static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t(int)>{};
  }

  template <class Receiver> struct op {
    using operation_state_concept = ex::operation_state_tag;
    int value;
    Receiver receiver;

    void start() & noexcept { ex::set_value(std::move(receiver), value); }
  };

  template <class Receiver> auto connect(Receiver receiver) && {
    return op<std::remove_cvref_t<Receiver>>{value, std::move(receiver)};
  }
};

int wait_one(auto sender) {
  auto result = ex::sync_wait(std::move(sender));
  c10::require(result.has_value(), "sender produced a value");
  return std::get<0>(*result);
}

} // namespace

int main() {
  return c10::test_main([] {
    auto default_path = ex::just(7) | ex::then([](int value) { return value * value; });
    int default_result = wait_one(std::move(default_path));

    domain_lowerings = 0;
    int custom_result = wait_one(square_sender{7});

    std::array<int, 8> slots{};
    auto bulk_sender =
        ex::just(0) |
        ex::bulk(ex::seq, static_cast<int>(slots.size()),
                 [&](int i, int &) { slots[static_cast<std::size_t>(i)] = i + 1; }) |
        ex::then([&](int seed) { return seed + std::accumulate(slots.begin(), slots.end(), 0); });
    int bulk_result = wait_one(std::move(bulk_sender));

    c10::require(default_result == 49, "default adaptor path result");
    c10::require(custom_result == 49, "custom domain path result");
    c10::require(domain_lowerings.load() == 1, "custom domain lowered exactly once");
    c10::require(bulk_result == 36, "bulk invoked each logical item once");

    std::cout << "default_path=then result=" << default_result << '\n';
    std::cout << "custom_domain=transform_sender lowerings=" << domain_lowerings.load()
              << " result=" << custom_result << '\n';
    std::cout << "bulk_policy=seq logical_items=" << slots.size() << " result=" << bulk_result
              << '\n';
  });
}
