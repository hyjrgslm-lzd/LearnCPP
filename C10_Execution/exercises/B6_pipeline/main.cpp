#include <c10/test.hpp>
#include <solution.hpp>
#include <algorithm>
#include <cmath>
#include <mutex>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace {
struct Expected {
  int valid{};
  int invalid{};
  double score{};
};

bool name_ok(std::string_view name) {
  if (name.empty())
    return false;
  auto good_first = [](unsigned char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_';
  };
  auto good_rest = [&](unsigned char c) { return good_first(c) || (c >= '0' && c <= '9'); };
  if (!good_first(static_cast<unsigned char>(name.front())))
    return false;
  for (char c : name.substr(1))
    if (!good_rest(static_cast<unsigned char>(c)))
      return false;
  return true;
}

bool parse_record(std::string_view text, std::string &name, int &value) {
  auto comma = text.find(',');
  if (comma == std::string_view::npos || text.find(',', comma + 1) != std::string_view::npos)
    return false;
  auto lhs = text.substr(0, comma);
  auto rhs = text.substr(comma + 1);
  if (!name_ok(lhs) || rhs.empty())
    return false;
  long long n = 0;
  for (char c : rhs) {
    if (c < '0' || c > '9')
      return false;
    n = n * 10 + (c - '0');
    if (n > 1000000)
      return false;
  }
  name = std::string(lhs);
  value = static_cast<int>(n);
  return true;
}

Expected oracle(const std::vector<std::string> &input) {
  Expected e{};
  for (const auto &s : input) {
    std::string name;
    int value = 0;
    if (parse_record(s, name, value)) {
      ++e.valid;
      e.score += value * 2.0 + static_cast<double>(name.size());
    } else {
      ++e.invalid;
    }
  }
  return e;
}

int count_phase(const c10_b6::Report &r, std::string_view phase) {
  return static_cast<int>(std::count_if(r.stages.begin(), r.stages.end(),
                                        [&](const auto &s) { return s.phase == phase; }));
}

std::set<std::thread::id> threads_for(const c10_b6::Report &r, std::string_view phase) {
  std::set<std::thread::id> ids;
  for (const auto &s : r.stages)
    if (s.phase == phase)
      ids.insert(s.thread);
  return ids;
}

std::vector<std::string> generated_records(int n) {
  std::vector<std::string> records;
  records.reserve(static_cast<std::size_t>(n));
  for (int i = 0; i < n; ++i) {
    int value = (i * 7919 + 17) % 1000001;
    records.push_back("Rec_" + std::to_string(i) + "," + std::to_string(value));
  }
  return records;
}

void check_pipeline(const std::vector<std::string> &input, std::size_t max_records) {
  std::mutex hook_lock;
  std::vector<std::thread::id> hook_threads;
  int hook_calls = 0;
  auto hook = [&](std::string_view name, int value) {
    std::lock_guard guard(hook_lock);
    ++hook_calls;
    hook_threads.push_back(std::this_thread::get_id());
    return value * 2.0 + static_cast<double>(name.size());
  };

  auto report = c10_b6::run_pipeline(input, max_records, hook);
  auto expected = oracle(input);
  c10::require(report.total == static_cast<int>(input.size()), "total input count recorded");
  c10::require(report.valid_count == expected.valid, "valid records counted");
  c10::require(report.invalid_count == expected.invalid, "ordinary parse failure counted");
  c10::require(std::abs(report.total_score - expected.score) < 0.0001,
               "merge sums injected enrich scores");
  c10::require(report.completed_batches == 3,
               "all three accepted branch batches complete before return");
  c10::require(report.merge_thread != report.caller_thread, "merge runs on merge scheduler");
  c10::require(count_phase(report, "parse") == static_cast<int>(input.size()),
               "one parse stage per accepted record");
  c10::require(count_phase(report, "enrich") == expected.valid,
               "one enrich stage per valid record");
  c10::require(hook_calls == expected.valid, "enrich callback called once per valid record");

  auto parse_threads = threads_for(report, "parse");
  auto enrich_threads = threads_for(report, "enrich");
  for (auto id : parse_threads)
    c10::require(id != report.caller_thread, "parse stages run on parse scheduler");
  for (auto id : enrich_threads)
    c10::require(id != report.caller_thread, "enrich stages run on enrich scheduler");
  for (auto p : parse_threads)
    for (auto e : enrich_threads)
      c10::require(p != e, "parse and enrich use distinct execution resources");
  for (auto id : hook_threads)
    c10::require(enrich_threads.contains(id), "enrich callback runs on recorded enrich resource");
}
} // namespace

int main() {
  return c10::test_main([] {
    check_pipeline(generated_records(1), 31);
    check_pipeline({}, 31);
    check_pipeline({"Zero,0", "One,1", "Top,999999", "Limit,1000000", "bad", "9bad,7", "Neg,-1",
                    "Huge,2147483648", "TooBig,1000001"},
                   31);
    for (int n = 2; n <= 31; ++n)
      check_pipeline(generated_records(n), 31);
    bool rejected = false;
    try {
      (void)c10_b6::run_pipeline(generated_records(4), 3);
    } catch (const std::invalid_argument &) {
      rejected = true;
    }
    c10::require(rejected, "bounded input rejects excess work");
  });
}
