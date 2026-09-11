#pragma once
#include <c10/native_io.hpp>
#include <c10/test.hpp>
#include <exec/async_scope.hpp>
#include <exec/static_thread_pool.hpp>
#include <stdexec/execution.hpp>

#include <atomic>
#include <charconv>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace c10_p1 {
namespace ex = stdexec;

struct record {
  std::string name;
  std::string category;
  std::int64_t value{};
};
struct report {
  int valid{};
  int invalid{};
  std::int64_t value_sum{};
  std::int64_t derived_sum{};
  std::map<std::string, int> by_category;
  int parse_tasks{};
  int compute_tasks{};
  int drain_tasks{};
  int env_queries{};
};
struct parsed_batch {
  std::vector<record> records;
  int invalid{};
};
struct pipeline_stopped : std::runtime_error {
  using std::runtime_error::runtime_error;
};
struct capacity_error : std::runtime_error {
  using std::runtime_error::runtime_error;
};

struct file_read_resources {
  c10_native::io_context io;
  std::shared_ptr<c10_native::native_file> file;
  std::vector<std::byte> bytes;
  std::atomic_int *env_queries{};
};

struct file_text_sender {
  using sender_concept = ex::sender_tag;
  std::filesystem::path path;
  std::shared_ptr<file_read_resources> resources;

  template <class Self, class... Env> static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t(std::string),
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
      shared_state(std::filesystem::path p, std::shared_ptr<file_read_resources> res, Receiver r)
          : path(std::move(p)), resources(std::move(res)), receiver(std::move(r)) {}
      std::filesystem::path path;
      std::shared_ptr<file_read_resources> resources;
      Receiver receiver;
      std::shared_ptr<c10_native::read_request> request;
      std::optional<stop_callback_t> on_stop;
      std::atomic_bool terminal{false};
      std::atomic_bool stop_requested{false};
      std::mutex request_mutex;

      void request_stop() noexcept {
        stop_requested.store(true, std::memory_order_release);
        std::shared_ptr<c10_native::read_request> local;
        {
          std::lock_guard lock{request_mutex};
          local = request;
        }
        if (local)
          local->cancel();
      }

      void complete_value(std::size_t n) noexcept {
        if (terminal.exchange(true, std::memory_order_acq_rel))
          return;
        on_stop.reset();
        std::string text{reinterpret_cast<const char *>(resources->bytes.data()), n};
        auto out = std::move(receiver);
        ex::set_value(std::move(out), std::move(text));
      }
      void complete_error(std::exception_ptr error) noexcept {
        if (terminal.exchange(true, std::memory_order_acq_rel))
          return;
        on_stop.reset();
        auto out = std::move(receiver);
        ex::set_error(std::move(out), std::move(error));
      }
      void complete_stopped() noexcept {
        if (terminal.exchange(true, std::memory_order_acq_rel))
          return;
        on_stop.reset();
        auto out = std::move(receiver);
        ex::set_stopped(std::move(out));
      }
    };

    std::shared_ptr<shared_state> state_;
    op(std::filesystem::path path, std::shared_ptr<file_read_resources> resources,
       Receiver receiver)
        : state_(std::make_shared<shared_state>(std::move(path), std::move(resources),
                                                std::move(receiver))) {}
    op(const op &) = delete;
    op(op &&) = delete;

    void start() & noexcept {
      auto state = state_;
      auto env = ex::get_env(state->receiver);
      if (state->resources->env_queries)
        state->resources->env_queries->fetch_add(1, std::memory_order_acq_rel);
      auto token = ex::get_stop_token(env);
      if (token.stop_requested()) {
        state->complete_stopped();
        return;
      }
      if constexpr (!ex::unstoppable_token<stop_token_t>)
        state->on_stop.emplace(token, stop_fn{state});
      if (!state->resources->io.available()) {
        if (state->resources->io.state() == c10_native::capability_state::unsupported) {
          state->complete_error(
              std::make_exception_ptr(c10::skip("native completion backend unsupported: " +
                                                state->resources->io.last_error().message())));
        } else {
          state->complete_error(std::make_exception_ptr(std::system_error(
              state->resources->io.last_error(), "native completion backend setup")));
        }
        return;
      }
      auto opened = state->resources->io.open_file(state->path);
      if (!opened) {
        state->complete_error(
            std::make_exception_ptr(std::system_error(opened.error(), "open records")));
        return;
      }
      state->resources->file = *opened;
      state->resources->bytes.resize(static_cast<std::size_t>(state->resources->file->size));
      if (state->resources->bytes.empty()) {
        state->complete_value(0);
        return;
      }
      auto submitted = state->resources->io.async_read_at(
          state->resources->file, 0, state->resources->bytes,
          [state](c10_native::completion event) noexcept {
            if (event.stopped) {
              state->complete_stopped();
              return;
            }
            if (event.error) {
              state->complete_error(std::make_exception_ptr(
                  std::system_error(event.error, "records read completion")));
              return;
            }
            if (event.bytes != state->resources->bytes.size()) {
              state->complete_error(
                  std::make_exception_ptr(std::runtime_error("records file changed during read")));
              return;
            }
            state->complete_value(event.bytes);
          });
      if (!submitted) {
        state->complete_error(
            std::make_exception_ptr(std::system_error(submitted.error(), "submit records read")));
        return;
      }
      {
        std::lock_guard lock{state->request_mutex};
        state->request = *submitted;
      }
      if (state->stop_requested.load(std::memory_order_acquire))
        state->request_stop();
    }
  };

  template <class Receiver> auto connect(Receiver receiver) const {
    return op<std::remove_cvref_t<Receiver>>{path, resources, std::move(receiver)};
  }
};

inline bool parse_line(std::string_view line, record &out) {
  const auto a = line.find(',');
  const auto b = a == std::string_view::npos ? a : line.find(',', a + 1);
  if (a == std::string_view::npos || b == std::string_view::npos)
    return false;
  out.name = std::string{line.substr(0, a)};
  out.category = std::string{line.substr(a + 1, b - a - 1)};
  const auto value = line.substr(b + 1);
  std::int64_t parsed{};
  auto [ptr, ec] = std::from_chars(value.data(), value.data() + value.size(), parsed);
  if (ec != std::errc{} || ptr != value.data() + value.size() || out.name.empty() ||
      out.category.empty())
    return false;
  if (parsed < -1000000 || parsed > 1000000)
    throw capacity_error("record value outside exercise bounds");
  out.value = parsed;
  return true;
}

inline parsed_batch parse_records(std::string_view text) {
  parsed_batch out;
  std::size_t start = 0;
  std::size_t lines = 0;
  while (start <= text.size()) {
    if (++lines > c10_native::max_record_lines)
      throw capacity_error("too many records");
    const auto end = text.find('\n', start);
    auto line =
        text.substr(start, end == std::string_view::npos ? text.size() - start : end - start);
    if (!line.empty() && line.back() == '\r')
      line.remove_suffix(1);
    if (!line.empty()) {
      record rec;
      if (parse_line(line, rec))
        out.records.push_back(std::move(rec));
      else
        ++out.invalid;
    }
    if (end == std::string_view::npos)
      break;
    start = end + 1;
  }
  return out;
}

inline auto run_pipeline_impl(const std::filesystem::path &path,
                              std::optional<ex::inplace_stop_token> stop_token) -> report {
  exec::static_thread_pool parse_pool{1};
  exec::static_thread_pool compute_pool{2};
  exec::async_scope scope;
  std::atomic_int parse_tasks{0};
  std::atomic_int compute_tasks{0};
  std::atomic_int drain_tasks{0};
  std::atomic_int env_queries{0};

  auto file_resources = std::make_shared<file_read_resources>();
  file_resources->env_queries = &env_queries;
  auto read_parse_graph = file_text_sender{path, file_resources} |
                          ex::continues_on(parse_pool.get_scheduler()) |
                          ex::then([&](std::string text) {
                            ++parse_tasks;
                            return parse_records(text);
                          }) |
                          ex::upon_error([&](std::exception_ptr error) {
                            ++parse_tasks;
                            try {
                              if (error)
                                std::rethrow_exception(error);
                            } catch (const capacity_error &) {
                              throw;
                            } catch (const std::system_error &) {
                              throw;
                            } catch (const pipeline_stopped &) {
                              throw;
                            } catch (...) {
                            }
                            return parsed_batch{{}, 1};
                          });

  auto parsed =
      stop_token ? ex::sync_wait(std::move(read_parse_graph) |
                                 ex::write_env(ex::env{ex::prop{ex::get_stop_token, *stop_token}}))
                 : ex::sync_wait(std::move(read_parse_graph));
  if (!parsed)
    throw pipeline_stopped("pipeline stopped before parse produced records");
  auto batch = std::move(std::get<0>(*parsed));
  auto records = std::make_shared<std::vector<record>>(std::move(batch.records));
  const int invalid = batch.invalid;

  auto sum_sender = ex::schedule(compute_pool.get_scheduler()) | ex::then([&, records] {
                      ++compute_tasks;
                      std::int64_t sum = 0;
                      for (const auto &r : *records)
                        sum += r.value;
                      return sum;
                    });
  auto derived_sender = ex::schedule(compute_pool.get_scheduler()) | ex::then([&, records] {
                          ++compute_tasks;
                          std::int64_t sum = 0;
                          for (const auto &r : *records)
                            sum += r.value * 2;
                          return sum;
                        });
  auto category_sender = ex::schedule(parse_pool.get_scheduler()) | ex::then([&, records] {
                           ++compute_tasks;
                           std::map<std::string, int> counts;
                           for (const auto &r : *records)
                             ++counts[r.category];
                           return counts;
                         });

  auto joined = ex::sync_wait(scope.nest(
      ex::when_all(std::move(sum_sender), std::move(derived_sender), std::move(category_sender))));
  (void)ex::sync_wait(scope.on_empty());
  ++drain_tasks;
  if (!joined)
    throw pipeline_stopped("pipeline stopped before compute produced report");
  return report{static_cast<int>(records->size()),
                invalid,
                std::get<0>(*joined),
                std::get<1>(*joined),
                std::get<2>(*joined),
                parse_tasks.load(),
                compute_tasks.load(),
                drain_tasks.load(),
                env_queries.load()};
}

inline auto run_pipeline(const std::filesystem::path &path) -> report {
  return run_pipeline_impl(path, std::nullopt);
}

inline auto run_pipeline(const std::filesystem::path &path, ex::inplace_stop_token stop_token)
    -> report {
  return run_pipeline_impl(path, stop_token);
}
} // namespace c10_p1
