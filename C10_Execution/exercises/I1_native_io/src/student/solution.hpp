#pragma once
#include <c10/native_io.hpp>
#include <c10/test.hpp>
#include <stdexec/execution.hpp>

#include <exception>
#include <type_traits>
#include <utility>

namespace c10_i1 {
namespace ex = stdexec;
using c10_native::io_context;
using c10_native::native_file;
using c10_native::read_result;

inline auto open_file(io_context &context, const std::filesystem::path &path) {
  return context.open_file(path);
}

struct unfinished_sender {
  using sender_concept = ex::sender_tag;
  template <class Self, class... Env> static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t(read_result),
                                     ex::set_error_t(std::exception_ptr), ex::set_stopped_t()>{};
  }
  template <class Receiver> struct op {
    using operation_state_concept = ex::operation_state_tag;
    Receiver receiver;
    void request_stop() noexcept {}
    void start() & noexcept {
      ex::set_error(std::move(receiver), std::make_exception_ptr(c10::unfinished(
                                             "I1 native_io: implement read sender bridge")));
    }
  };
  template <class Receiver> auto connect(Receiver receiver) const {
    return op<std::remove_cvref_t<Receiver>>{std::move(receiver)};
  }
};

inline auto read_at(std::shared_ptr<native_file> file, std::uint64_t offset,
                    std::span<std::byte> buffer) {
  (void)file;
  (void)offset;
  (void)buffer;
  return unfinished_sender{};
}
} // namespace c10_i1
