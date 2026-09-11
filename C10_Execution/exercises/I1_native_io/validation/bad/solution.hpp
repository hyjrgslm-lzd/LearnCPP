#pragma once
#include <c10/native_io.hpp>
#include <stdexec/execution.hpp>

#include <atomic>
#include <exception>
#include <memory>
#include <optional>
#include <span>
#include <system_error>
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

struct read_sender {
  using sender_concept = ex::sender_tag;
  std::shared_ptr<native_file> file;
  std::uint64_t offset{};
  std::span<std::byte> buffer;

  template <class Self, class... Env> static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t(read_result),
                                     ex::set_error_t(std::exception_ptr), ex::set_stopped_t()>{};
  }

  template <class Receiver> struct op {
    using operation_state_concept = ex::operation_state_tag;
    struct shared_state;
    struct stop_fn {
      std::weak_ptr<shared_state> state;
      void operator()() const noexcept {
        if (auto locked = state.lock())
          locked->request_stop();
      }
    };
    using stop_token_t = ex::stop_token_of_t<ex::env_of_t<Receiver>>;
    using stop_callback_t = ex::stop_callback_for_t<stop_token_t, stop_fn>;

    struct shared_state {
      std::shared_ptr<native_file> file;
      std::uint64_t offset{};
      std::span<std::byte> buffer;
      Receiver receiver;
      std::shared_ptr<c10_native::read_request> request;
      std::optional<stop_callback_t> on_stop;
      std::atomic_bool stop_requested{false};
      std::atomic_bool terminal{false};

      shared_state(std::shared_ptr<native_file> f, std::uint64_t off, std::span<std::byte> buf,
                   Receiver r)
          : file(std::move(f)), offset(off), buffer(buf), receiver(std::move(r)) {}

      void request_stop() noexcept {
        stop_requested.store(true, std::memory_order_release);
        if (request)
          request->cancel();
      }

      void complete(c10_native::completion event) noexcept {
        if (terminal.exchange(true, std::memory_order_acq_rel))
          return;
        on_stop.reset();
        auto out = std::move(receiver);
        if (event.stopped) {
          ex::set_stopped(std::move(out));
        } else if (event.error) {
          ex::set_error(std::move(out), std::make_exception_ptr(std::system_error(
                                            event.error, "native read completion")));
        } else {
          ex::set_value(std::move(out), read_result{event.bytes});
        }
      }
    };

    std::shared_ptr<shared_state> state_;

    op(std::shared_ptr<native_file> file, std::uint64_t offset, std::span<std::byte> buffer,
       Receiver receiver)
        : state_(std::make_shared<shared_state>(std::move(file), offset, buffer,
                                                std::move(receiver))) {}
    op(const op &) = delete;
    op(op &&) = delete;

    void start() & noexcept {
      auto state = state_;
      auto token = ex::get_stop_token(ex::get_env(state->receiver));
      if (token.stop_requested()) {
        state->complete(c10_native::completion{0, {}, true});
        return;
      }
      if constexpr (!ex::unstoppable_token<stop_token_t>) {
        state->on_stop.emplace(token, stop_fn{state});
      }
      auto submitted = state->file->context->async_read_at(
          state->file, state->offset, state->buffer,
          [state](c10_native::completion event) noexcept { state->complete(event); });
      if (!submitted) {
        state->complete(c10_native::completion{0, submitted.error(), false});
        return;
      }
      state->request = *submitted;
      if (state->stop_requested.load(std::memory_order_acquire))
        state->request->cancel();
    }

    void request_stop() noexcept { state_->request_stop(); }
  };

  template <class Receiver> auto connect(Receiver receiver) const {
    return op<std::remove_cvref_t<Receiver>>{file, offset, buffer, std::move(receiver)};
  }
};

inline auto read_at(std::shared_ptr<native_file> file, std::uint64_t offset,
                    std::span<std::byte> buffer) {
  (void)offset;
  return read_sender{std::move(file), 0, buffer};
}
} // namespace c10_i1
