#include <c10/test.hpp>
#include <c10/native_io.hpp>
#include <solution.hpp>
#include <stdexec/execution.hpp>

#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>

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

auto temp_file() {
#ifdef _WIN32
  const auto pid = ::GetCurrentProcessId();
#else
  const auto pid = ::getpid();
#endif
  return std::filesystem::temp_directory_path() / ("c10_p1_" + std::to_string(pid) + ".csv");
}

void write_text(const std::filesystem::path &path, std::string_view text) {
  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  out << text;
  out.close();
  c10::require(out.good(), "text fixture written");
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

void write_fixture(const std::filesystem::path &path) {
  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  out << "alpha,core,10\n";
  out << "bad line\n";
  out << "beta,io,7\n";
  out << "gamma,core,5\n";
  out.close();
  c10::require(out.good(), "fixture written");
}

void check_report() {
  const auto path = temp_file();
  struct cleanup {
    std::filesystem::path path;
    ~cleanup() {
      std::error_code ignored;
      std::filesystem::remove(path, ignored);
    }
  } remove_payload{path};
  write_fixture(path);

  auto report = c10_p1::run_pipeline(path);
  c10::require(report.valid == 3, "valid records counted");
  c10::require(report.invalid == 1, "invalid records recovered");
  c10::require(report.value_sum == 22, "value branch sum");
  c10::require(report.derived_sum == 44, "derived branch sum");
  c10::require(report.by_category.at("core") == 2 && report.by_category.at("io") == 1,
               "category branch counts");
  c10::require(report.parse_tasks >= 1 && report.compute_tasks >= 3 && report.drain_tasks >= 1,
               "pipeline stage counters recorded lower-bound execution");
  c10::require(report.env_queries >= 1,
               "file read sender queried receiver stop token from environment");
}

void check_stop_and_capacity_boundaries() {
  const auto path = temp_file();
  struct cleanup {
    std::filesystem::path path;
    ~cleanup() {
      std::error_code ignored;
      std::filesystem::remove(path, ignored);
    }
  } remove_payload{path};
  write_fixture(path);

  ex::inplace_stop_source source;
  source.request_stop();
  require_throws<c10_p1::pipeline_stopped>(
      [&] { (void)c10_p1::run_pipeline(path, source.get_token()); },
      "requested stop token stops pipeline before report");

  const auto huge_path = path.parent_path() / (path.stem().string() + "_huge.csv");
  struct cleanup_huge {
    std::filesystem::path path;
    ~cleanup_huge() {
      std::error_code ignored;
      std::filesystem::remove(path, ignored);
    }
  } remove_huge{huge_path};
  write_sparse_size(huge_path, c10_native::max_read_bytes + 1);
  require_throws<std::system_error>([&] { (void)c10_p1::run_pipeline(huge_path); },
                                    "oversized input file is rejected");

  std::string too_many;
  for (std::size_t i = 0; i != c10_native::max_record_lines + 1; ++i)
    too_many += "n,cat,1\n";
  write_text(path, too_many);
  require_throws<c10_p1::capacity_error>([&] { (void)c10_p1::run_pipeline(path); },
                                         "too many records is rejected");

  write_text(path, "big,cat,1000001\n");
  require_throws<c10_p1::capacity_error>([&] { (void)c10_p1::run_pipeline(path); },
                                         "value upper bound is rejected");
}

} // namespace

int main() {
  return c10::test_main([] {
    check_report();
    check_stop_and_capacity_boundaries();
  });
}
