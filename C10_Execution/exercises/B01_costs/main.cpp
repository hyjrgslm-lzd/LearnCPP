#include <c10/test.hpp>
#include <stdexec/execution.hpp>
#include <exec/async_scope.hpp>
#include <exec/static_thread_pool.hpp>
#include <chrono>
#include <charconv>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <string>
#include <string_view>
#include <vector>

namespace ex = stdexec;
using clock_type = std::chrono::steady_clock;

std::uint64_t transform(std::uint64_t value, unsigned work) noexcept {
  for (unsigned i = 0; i < work; ++i)
    value = value * value + 3 * value + 7;
  return value;
}
std::uint64_t oracle(std::uint64_t value, unsigned work) noexcept {
  for (unsigned i = 0; i < work; ++i)
    value = (value + 1) * (value + 2) + 5;
  return value;
}
struct result {
  double seconds;
  double pre_wait_seconds;
  std::uint64_t checksum;
  std::size_t submissions;
};
result run(std::string_view mode, std::size_t size, unsigned work, std::uint64_t seed) {
  c10::require(mode == "serial" || mode == "per_item" || mode == "bulk",
               "unknown benchmark variant");
  c10::require(size <= 262144 && work <= 4096, "bounded size/work exceeded");
  std::vector<std::uint64_t> input(size), output(size, 0), expected(size);
  for (std::size_t i = 0; i < size; ++i) {
    seed = seed * 6364136223846793005ULL + 1442695040888963407ULL;
    input[i] = seed;
    expected[i] = oracle(seed, work);
  }
  // The pool is destroyed before the borrowed buffers. Construction and final
  // thread shutdown are outside the measured completion interval in every mode.
  exec::static_thread_pool pool(2);
  auto scheduler = pool.get_scheduler();
  exec::async_scope scope;
  const auto begin = clock_type::now();
  auto submitted = begin;
  std::size_t submissions = 0;
  if (mode == "serial") {
    for (std::size_t i = 0; i < size; ++i)
      output[i] = transform(input[i], work);
  } else if (mode == "per_item") {
    try {
      for (std::size_t i = 0; i < size; ++i) {
        scope.spawn(ex::schedule(scheduler) |
                    ex::then([&, i] { output[i] = transform(input[i], work); }));
        ++submissions;
      }
    } catch (...) {
      ex::sync_wait(scope.on_empty()); // Retire accepted work before unwinding its buffers.
      throw;
    }
    submitted = clock_type::now();
    ex::sync_wait(scope.on_empty());
  } else {
    // One logical submission. bulk may create multiple internal tasks;
    // submissions deliberately does not claim to count those implementation details.
    auto task = ex::schedule(scheduler) | ex::bulk(ex::par, size, [&](std::size_t i) noexcept {
                  output[i] = transform(input[i], work);
                });
    ++submissions;
    submitted = clock_type::now();
    ex::sync_wait(std::move(task));
  }
  const auto end = clock_type::now();
  c10::require(output == expected, "every output must match independent polynomial oracle");
  return {std::chrono::duration<double>(end - begin).count(),
          std::chrono::duration<double>(submitted - begin).count(),
          std::accumulate(output.begin(), output.end(), std::uint64_t{}), submissions};
}
std::uint64_t argument(std::string_view text) {
  std::uint64_t value{};
  const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
  c10::require(error == std::errc{} && end == text.data() + text.size(),
               "expected unsigned integer argument");
  return value;
}
int main(int argc, char **argv) {
  return c10::test_main([&] {
    if (argc == 1 || (argc == 2 && std::string_view(argv[1]) == "--self-test")) {
      for (std::size_t n : {0u, 1u, 7u, 33u}) {
        const auto baseline = run("serial", n, 17, 42);
        for (std::string_view mode : {"per_item", "bulk"}) {
          const auto candidate = run(mode, n, 17, 42);
          c10::require(candidate.checksum == baseline.checksum, "variants share one contract");
          c10::require(candidate.submissions == (mode == "per_item" ? n : 1),
                       "logical submission count");
        }
      }
      return;
    }
    c10::require(argc == 6 && std::string_view(argv[1]) == "--bench",
                 "usage: B01_costs --bench serial|per_item|bulk size work seed");
    const std::string mode = argv[2];
    const auto size = argument(argv[3]);
    const auto work = argument(argv[4]);
    const auto seed = argument(argv[5]);
    c10::require(size > 0 && size <= 262144 && work <= 4096, "benchmark bounds");
    const auto measured =
        run(mode, static_cast<std::size_t>(size), static_cast<unsigned>(work), seed);
    std::cout.precision(12);
    std::cout << "{\"variant\":\"" << mode << "\",\"size\":" << size << ",\"work\":" << work
              << ",\"seed\":" << seed << ",\"seconds\":" << measured.seconds
              << ",\"pre_wait_seconds\":" << measured.pre_wait_seconds
              << ",\"logical_submissions\":" << measured.submissions
              << ",\"checksum\":" << measured.checksum << ",\"configured_pool_threads\":2}\n";
  });
}
