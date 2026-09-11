#include <c10/test.hpp>
#include <solution.hpp>
#include <stdexec/execution.hpp>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <functional>
#include <mutex>
#include <span>
#include <string>
#include <system_error>
#include <tuple>
#include <utility>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace ex = stdexec;

namespace {

auto temp_file(std::string_view name) {
#ifdef _WIN32
  const auto pid = ::GetCurrentProcessId();
#else
  const auto pid = ::getpid();
#endif
  return std::filesystem::temp_directory_path() /
         ("c10_i1_" + std::to_string(pid) + "_" + std::string{name});
}

void write_text(const std::filesystem::path &path, std::string_view text) {
  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  out << text;
  out.close();
  c10::require(out.good(), "fixture file written");
}

void write_sparse_size(const std::filesystem::path &path, std::uint64_t size) {
  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  out.seekp(static_cast<std::streamoff>(size - 1));
  out.put('\0');
  out.close();
  c10::require(out.good(), "sparse fixture written");
}

template <class Exception, class Fn> void require_throws(Fn &&fn, std::string_view message) {
  bool thrown = false;
  try {
    fn();
  } catch (const Exception &) {
    thrown = true;
  }
  c10::require(thrown, message);
}

std::string as_string(std::span<const std::byte> bytes, std::size_t n) {
  return {reinterpret_cast<const char *>(bytes.data()), n};
}

struct shared_state {
  std::mutex mutex;
  std::condition_variable cv;
  int completions{};
  std::string channel;
  std::size_t bytes{};
};

struct receiver {
  using receiver_concept = ex::receiver_tag;
  std::shared_ptr<shared_state> state;

  void set_value(c10_native::read_result result) noexcept {
    std::lock_guard lock{state->mutex};
    ++state->completions;
    state->channel = "value";
    state->bytes = result.bytes;
    state->cv.notify_all();
  }
  void set_error(std::exception_ptr) noexcept {
    std::lock_guard lock{state->mutex};
    ++state->completions;
    state->channel = "error";
    state->cv.notify_all();
  }
  void set_stopped() noexcept {
    std::lock_guard lock{state->mutex};
    ++state->completions;
    state->channel = "stopped";
    state->cv.notify_all();
  }
};

struct stopped_env_receiver : receiver {
  std::shared_ptr<ex::inplace_stop_source> source;
  auto get_env() const noexcept {
    return ex::env{ex::prop{ex::get_stop_token, source->get_token()}};
  }
};

struct deleting_state {
  std::mutex mutex;
  std::condition_variable cv;
  int completions{};
  std::string channel;
  std::size_t bytes{};
  std::function<void()> destroy;
};

struct deleting_receiver {
  using receiver_concept = ex::receiver_tag;
  std::shared_ptr<deleting_state> state;

  void finish(std::string channel, std::size_t bytes = 0) noexcept {
    std::function<void()> destroy_now;
    {
      std::lock_guard lock{state->mutex};
      ++state->completions;
      state->channel = std::move(channel);
      state->bytes = bytes;
      destroy_now = std::move(state->destroy);
      state->cv.notify_all();
    }
    if (destroy_now)
      destroy_now();
  }
  void set_value(c10_native::read_result result) noexcept { finish("value", result.bytes); }
  void set_error(std::exception_ptr) noexcept { finish("error"); }
  void set_stopped() noexcept { finish("stopped"); }
};

template <class Sender>
void start_heap_operation(Sender &&sender, const std::shared_ptr<deleting_state> &state) {
  using op_t = decltype(ex::connect(std::forward<Sender>(sender), deleting_receiver{state}));
  auto *op = new op_t(ex::connect(std::forward<Sender>(sender), deleting_receiver{state}));
  {
    std::lock_guard lock{state->mutex};
    state->destroy = [op] { delete op; };
  }
  ex::start(*op);
}

void require_context_ready(c10_i1::io_context &context) {
  if (context.available())
    return;
  if (context.state() == c10_native::capability_state::unsupported) {
    throw c10::skip("native completion backend unsupported: " + context.last_error().message());
  }
  throw std::system_error(context.last_error(), "native completion backend setup");
}

void check_boundaries() {
  c10_i1::io_context context;
  require_context_ready(context);

  const auto path = temp_file("payload.txt");
  struct cleanup {
    std::filesystem::path path;
    ~cleanup() {
      std::error_code ignored;
      std::filesystem::remove(path, ignored);
    }
  } remove_payload{path};
  write_text(path, "alpha\nbeta\n");

  auto missing = c10_i1::open_file(context, path.string() + ".missing");
  c10::require(!missing, "missing file reports open error");

  auto file = c10_i1::open_file(context, path);
  c10::require(file.has_value(), "native file opens");

  std::vector<std::byte> first(5);
  auto first_result = ex::sync_wait(c10_i1::read_at(*file, 0, first));
  c10::require(first_result && std::get<0>(*first_result).bytes == 5, "first read completes");
  c10::require(as_string(first, 5) == "alpha", "first read bytes match");

  std::vector<std::byte> tail(5);
  auto tail_result = ex::sync_wait(c10_i1::read_at(*file, 6, tail));
  c10::require(tail_result && std::get<0>(*tail_result).bytes == 5, "offset read returns tail");
  c10::require(as_string(tail, 5) == "beta\n", "offset read bytes match");

  std::vector<std::byte> short_read(16);
  auto short_result = ex::sync_wait(c10_i1::read_at(*file, 6, short_read));
  c10::require(short_result && std::get<0>(*short_result).bytes == 5,
               "short read reports actual bytes");

  bool out_of_range_rejected = false;
  try {
    std::vector<std::byte> eof(4);
    (void)ex::sync_wait(c10_i1::read_at(*file, 99, eof));
  } catch (const std::system_error &) {
    out_of_range_rejected = true;
  }
  c10::require(out_of_range_rejected, "out-of-range offset is rejected before submit");

  context.close();
  auto closed = c10_i1::open_file(context, path);
  c10::require(!closed, "closed context rejects new opens");
}

void check_multi_and_cancel_race() {
  c10_i1::io_context context;
  require_context_ready(context);
  const auto path = temp_file("race.txt");
  struct cleanup {
    std::filesystem::path path;
    ~cleanup() {
      std::error_code ignored;
      std::filesystem::remove(path, ignored);
    }
  } remove_payload{path};
  write_text(path, std::string(4096, 'x'));
  auto file = c10_i1::open_file(context, path);
  c10::require(file.has_value(), "race file opens");

  std::vector<std::byte> a(32), b(32);
  auto both =
      ex::sync_wait(ex::when_all(c10_i1::read_at(*file, 0, a), c10_i1::read_at(*file, 32, b)));
  c10::require(both.has_value(), "multiple in-flight reads complete");
  c10::require(std::get<0>(*both).bytes == 32 && std::get<1>(*both).bytes == 32,
               "multiple reads keep identity");

  std::vector<std::byte> cancel_buffer(4096);
  auto state = std::make_shared<shared_state>();
  auto op = ex::connect(c10_i1::read_at(*file, 0, cancel_buffer), receiver{state});
  ex::start(op);
  op.request_stop();
  std::unique_lock lock{state->mutex};
  const bool done =
      state->cv.wait_for(lock, std::chrono::seconds(5), [&] { return state->completions != 0; });
  c10::require(done, "cancel race reaches one terminal signal");
  c10::require(state->completions == 1, "cancel race has exactly one completion");
  c10::require(state->channel == "value" || state->channel == "stopped",
               "cancel race reports target result or stopped");

  auto stopped_state = std::make_shared<shared_state>();
  auto source = std::make_shared<ex::inplace_stop_source>();
  std::vector<std::byte> prestop(8);
  auto stopped_op = ex::connect(c10_i1::read_at(*file, 0, prestop),
                                stopped_env_receiver{{stopped_state}, source});
  source->request_stop();
  ex::start(stopped_op);
  std::unique_lock stopped_lock{stopped_state->mutex};
  c10::require(stopped_state->completions == 1 && stopped_state->channel == "stopped",
               "receiver stop_token requested before start maps to stopped");
}

void check_terminal_can_destroy_operation() {
  c10_i1::io_context context;
  require_context_ready(context);
  const auto path = temp_file("destroy.txt");
  struct cleanup {
    std::filesystem::path path;
    ~cleanup() {
      std::error_code ignored;
      std::filesystem::remove(path, ignored);
    }
  } remove_payload{path};
  write_text(path, "abcdef");
  auto file = c10_i1::open_file(context, path);
  c10::require(file.has_value(), "destroy file opens");

  auto wait_done = [](const std::shared_ptr<deleting_state> &state, std::string_view expected,
                      std::string_view label) {
    std::unique_lock lock{state->mutex};
    const bool done =
        state->cv.wait_for(lock, std::chrono::seconds(5), [&] { return state->completions != 0; });
    c10::require(done, std::string{label} + " reaches terminal");
    c10::require(state->completions == 1, std::string{label} + " has one terminal");
    c10::require(state->channel == expected, std::string{label} + " terminal channel");
  };

  std::vector<std::byte> empty;
  auto zero = std::make_shared<deleting_state>();
  start_heap_operation(c10_i1::read_at(*file, 0, std::span<std::byte>{empty}), zero);
  wait_done(zero, "value", "zero-length self-destroy");

  std::vector<std::byte> payload(3);
  auto normal = std::make_shared<deleting_state>();
  start_heap_operation(c10_i1::read_at(*file, 1, payload), normal);
  wait_done(normal, "value", "async self-destroy");

  std::vector<std::byte> bad(1);
  auto error = std::make_shared<deleting_state>();
  start_heap_operation(c10_i1::read_at(*file, 999, bad), error);
  wait_done(error, "error", "submit-error self-destroy");
}

void check_resource_boundaries() {
  c10_i1::io_context context;
  require_context_ready(context);
  const auto path = temp_file("limits.txt");
  const auto huge_path = temp_file("huge.bin");
  struct cleanup {
    std::filesystem::path a;
    std::filesystem::path b;
    ~cleanup() {
      std::error_code ignored;
      std::filesystem::remove(a, ignored);
      std::filesystem::remove(b, ignored);
    }
  } remove_payload{path, huge_path};
  write_text(path, "abc");
  auto file = c10_i1::open_file(context, path);
  c10::require(file.has_value(), "limits file opens");

  std::vector<std::byte> eof(1);
  auto eof_result = ex::sync_wait(c10_i1::read_at(*file, (*file)->size, eof));
  c10::require(eof_result && std::get<0>(*eof_result).bytes == 0,
               "read exactly at EOF completes with zero bytes");

  std::vector<std::byte> oversized(c10_native::max_read_bytes + 1);
  require_throws<std::system_error>(
      [&] { (void)ex::sync_wait(c10_i1::read_at(*file, 0, oversized)); },
      "oversized read buffer is rejected");

  auto directory = c10_i1::open_file(context, std::filesystem::temp_directory_path());
  c10::require(!directory, "non-regular directory path is rejected");

  write_sparse_size(huge_path, c10_native::max_read_bytes + 1);
  auto huge = c10_i1::open_file(context, huge_path);
  c10::require(!huge, "oversized regular file is rejected");
}

} // namespace

int main() {
  return c10::test_main([] {
    check_boundaries();
    check_multi_and_cancel_race();
    check_terminal_can_destroy_operation();
    check_resource_boundaries();
  });
}
