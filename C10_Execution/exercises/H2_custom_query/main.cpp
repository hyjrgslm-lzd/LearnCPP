#include <c10/test.hpp>
#include <solution.hpp>
#include <stdexec/execution.hpp>
#include <string>
#include <tuple>
#include <type_traits>
namespace ex = stdexec;
struct absent_query {
  template <class E> auto operator()(const E &e) const -> decltype(e.query(*this));
};
template <class E>
concept has_absent_query = requires(const E &e) { absent_query{}(e); };
int main() {
  return c10::test_main([] {
    ex::run_loop loop;
    for (int quota : {0, 1, 7, 31}) {
      auto base = ex::env{ex::prop{ex::get_scheduler, loop.get_scheduler()},
                          ex::prop{c10_h2::get_quota, quota}, c10_h2::trace_env{"parent"}};
      auto text = std::string("request-") + std::to_string(quota);
      auto child = c10_h2::make_override_env(base, c10_h2::trace_env{text});
      c10::require(c10_h2::get_trace_id(child) == text, "override takes priority over parent");
      c10::require(c10_h2::get_trace_id(base) == "parent", "parent remains unchanged");
      c10::require(c10_h2::get_quota(child) == quota, "unrelated custom query falls back");
      c10::require(ex::get_scheduler(child) == loop.get_scheduler(),
                   "real scheduler query falls back");
      static_assert(!has_absent_query<decltype(child)>);
      static_assert(noexcept(c10_h2::get_trace_id(child)));
      static_assert(std::is_same_v<decltype(c10_h2::get_trace_id(child)), const std::string &>);
      auto nested = c10_h2::make_override_env(child, ex::prop{c10_h2::get_quota, quota + 2});
      auto graph = ex::write_env(
          ex::when_all(ex::read_env(c10_h2::get_trace_id) |
                           ex::then([](const std::string &v) { return std::string(v); }),
                       ex::read_env(c10_h2::get_quota)),
          nested);
      auto result = ex::sync_wait(std::move(graph));
      c10::require(result && std::get<0>(*result) == text && std::get<1>(*result) == quota + 2,
                   "sender reads composed receiver environment");
    }
  });
}
