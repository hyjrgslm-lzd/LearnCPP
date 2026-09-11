#pragma once
#include <c10/native_io.hpp>
#include <c10/test.hpp>
#include <exec/async_scope.hpp>
#include <exec/static_thread_pool.hpp>
#include <stdexec/execution.hpp>

#include <atomic>
#include <charconv>
#include <condition_variable>
#include <cstdint>
#include <exception>
#include <map>
#include <mutex>
#include <string>
#include <string_view>
#include <system_error>
#include <tuple>
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

inline auto read_text_native(const std::filesystem::path &path) -> std::string {
  c10_native::io_context io;
  if (!io.available()) {
    if (io.state() == c10_native::capability_state::unsupported)
      throw c10::skip("native completion backend unsupported: " + io.last_error().message());
    throw std::system_error(io.last_error(), "native completion backend setup");
  }
  auto file = io.open_file(path);
  if (!file)
    throw std::system_error(file.error(), "open records");
  std::vector<std::byte> bytes(static_cast<std::size_t>((*file)->size));
  if (bytes.empty())
    return {};
  std::mutex mutex;
  std::condition_variable cv;
  bool done = false;
  c10_native::completion event;
  auto request = io.async_read_at(*file, 0, bytes, [&](c10_native::completion completed) noexcept {
    std::lock_guard lock{mutex};
    event = completed;
    done = true;
    cv.notify_one();
  });
  if (!request)
    throw std::system_error(request.error(), "submit records read");
  std::unique_lock lock{mutex};
  cv.wait(lock, [&] { return done; });
  if (event.stopped)
    throw std::runtime_error("records read stopped");
  if (event.error)
    throw std::system_error(event.error, "records read completion");
  if (event.bytes != bytes.size())
    throw std::runtime_error("records file changed during read");
  return {reinterpret_cast<const char *>(bytes.data()), bytes.size()};
}

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
    return false;
  out.value = parsed;
  return true;
}

inline parsed_batch parse_records(std::string_view text) {
  parsed_batch out;
  std::size_t start = 0;
  std::size_t lines = 0;
  while (start <= text.size()) {
    if (++lines > c10_native::max_record_lines)
      throw std::runtime_error("too many records");
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

inline auto run_pipeline(const std::filesystem::path &path) -> report {
  auto text = read_text_native(path);
  exec::static_thread_pool parse_pool{1};
  exec::static_thread_pool compute_pool{2};
  exec::async_scope scope;
  std::atomic_int parse_tasks{0};
  std::atomic_int compute_tasks{0};
  std::atomic_int drain_tasks{0};
  std::atomic_int env_queries{0};

  auto parsed_sender = ex::schedule(parse_pool.get_scheduler()) |
                       ex::then([&, text = std::move(text)] {
                         ++parse_tasks;
                         ++env_queries;
                         return parse_records(text);
                       }) |
                       ex::upon_error([&](std::exception_ptr) noexcept {
                         ++parse_tasks;
                         return parsed_batch{{}, 1};
                       });
  auto parsed = ex::sync_wait(std::move(parsed_sender));
  if (!parsed)
    throw std::runtime_error("parse stopped");
  auto batch = std::move(std::get<0>(*parsed));
  auto records = batch.records;

  auto sum_sender = ex::schedule(compute_pool.get_scheduler()) | ex::then([&, records] {
                      ++compute_tasks;
                      std::int64_t sum = 0;
                      for (const auto &r : records)
                        sum += r.value;
                      return sum;
                    });
  auto derived_sender = ex::schedule(compute_pool.get_scheduler()) | ex::then([&, records] {
                          ++compute_tasks;
                          std::int64_t sum = 0;
                          for (const auto &r : records)
                            sum += r.value * 2;
                          return sum;
                        });
  auto category_sender = ex::schedule(parse_pool.get_scheduler()) | ex::then([&, records] {
                           ++compute_tasks;
                           std::map<std::string, int> counts;
                           for (const auto &r : records)
                             ++counts[r.category];
                           return counts;
                         });

  auto joined = ex::sync_wait(scope.nest(
      ex::when_all(std::move(sum_sender), std::move(derived_sender), std::move(category_sender))));
  (void)ex::sync_wait(scope.on_empty());
  ++drain_tasks;
  if (!joined)
    throw std::runtime_error("compute stopped");
  return report{
      static_cast<int>(records.size()),
      0, // Deliberate fault: discard the invalid-record count through the real public entry.
      std::get<0>(*joined),
      std::get<1>(*joined),
      std::get<2>(*joined),
      parse_tasks.load(),
      compute_tasks.load(),
      drain_tasks.load(),
      env_queries.load()};
}

inline auto run_pipeline(const std::filesystem::path &path, ex::inplace_stop_token) -> report {
  if (path.empty())
    throw pipeline_stopped("bad implementation does not connect receiver stop token");
  return run_pipeline(path);
}
} // namespace c10_p1
