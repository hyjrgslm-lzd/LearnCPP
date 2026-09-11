#pragma once
#include <stdexec/execution.hpp>
#include <exec/static_thread_pool.hpp>
#include <array>
#include <charconv>
#include <functional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>
namespace c10_b6 {
namespace ex = stdexec;
struct StageRecord {
  std::string phase;
  std::string record;
  int branch{};
  std::thread::id thread{};
};
struct Report {
  int total{};
  int valid_count{};
  int invalid_count{};
  double total_score{};
  int completed_batches{};
  std::thread::id caller_thread{};
  std::thread::id merge_thread{};
  std::vector<StageRecord> stages;
};
using EnrichHook = std::function<double(std::string_view, int)>;
struct Item {
  std::string raw;
  std::string name;
  int value{};
  bool valid{};
  double score{};
  std::vector<StageRecord> stages;
};
struct Batch {
  int branch{};
  std::vector<Item> items;
};
inline bool name_ok(std::string_view name) {
  if (name.empty())
    return false;
  auto first = [](unsigned char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_';
  };
  auto rest = [&](unsigned char c) { return first(c) || (c >= '0' && c <= '9'); };
  if (!first(static_cast<unsigned char>(name.front())))
    return false;
  for (char c : name.substr(1))
    if (!rest(static_cast<unsigned char>(c)))
      return false;
  return true;
}
inline Item parse_one(std::string raw, int branch) {
  Item item{};
  item.raw = std::move(raw);
  item.stages.push_back({"parse", item.raw, branch, std::this_thread::get_id()});
  auto text = std::string_view(item.raw);
  auto comma = text.find(',');
  if (comma == std::string_view::npos || text.find(',', comma + 1) != std::string_view::npos)
    return item;
  auto name = text.substr(0, comma);
  auto digits = text.substr(comma + 1);
  if (!name_ok(name) || digits.empty())
    return item;
  int value{};
  auto [ptr, ec] = std::from_chars(digits.data(), digits.data() + digits.size(), value);
  if (ec != std::errc{} || ptr != digits.data() + digits.size() || value < 0 || value > 1000000)
    return item;
  item.name = std::string(name);
  item.value = value;
  item.valid = true;
  return item;
}
inline Item enrich_one(Item item, int branch, EnrichHook &hook) {
  if (!item.valid)
    return item;
  item.score = hook(item.name, item.value);
  item.stages.push_back({"enrich", item.name, branch, std::this_thread::get_id()});
  return item;
}
inline Batch parse_batch(std::vector<std::string> records, int branch) {
  Batch batch{};
  batch.branch = branch;
  for (auto &raw : records)
    batch.items.push_back(parse_one(std::move(raw), branch));
  return batch;
}
inline Batch enrich_batch(Batch batch, EnrichHook &hook) {
  for (auto &item : batch.items)
    item = enrich_one(std::move(item), batch.branch, hook);
  return batch;
}
inline Report merge_batches(std::array<Batch, 3> batches, std::thread::id caller, int total) {
  Report report{};
  report.total = total;
  report.caller_thread = caller;
  report.merge_thread = std::this_thread::get_id();
  report.completed_batches = 3;
  for (auto &batch : batches)
    for (auto &item : batch.items) {
      for (auto &stage : item.stages)
        report.stages.push_back(std::move(stage));
      if (item.valid) {
        ++report.valid_count;
        report.total_score += item.score;
      } else {
        ++report.invalid_count;
      }
    }
  return report;
}
inline double default_score(std::string_view name, int value) {
  return value * 1.5 + static_cast<double>(name.size());
}
inline Report run_pipeline(const std::vector<std::string> &input, std::size_t max_records,
                           EnrichHook hook) {
  if (input.size() > max_records)
    throw std::invalid_argument("too many records");
  std::array<std::vector<std::string>, 3> buckets;
  for (std::size_t i = 0; i < input.size(); ++i)
    buckets[i % buckets.size()].push_back(input[i]);
  auto caller = std::this_thread::get_id();
  exec::static_thread_pool parse_pool(1), enrich_pool(1), merge_pool(1);
  auto parse_sch = parse_pool.get_scheduler();
  auto enrich_sch = enrich_pool.get_scheduler();
  auto merge_sch = merge_pool.get_scheduler();
  auto branch = [&](int id) {
    return ex::starts_on(
        parse_sch, ex::just(std::move(buckets[id]), id) |
                       ex::then([](std::vector<std::string> records, int branch_id) {
                         return parse_batch(std::move(records), branch_id);
                       }) |
                       ex::continues_on(enrich_sch) |
                       ex::then([&](Batch batch) { return enrich_batch(std::move(batch), hook); }));
  };
  auto graph =
      ex::when_all(branch(0), branch(1), branch(2)) | ex::continues_on(merge_sch) |
      ex::then([=](Batch a, Batch b, Batch c) mutable {
        return merge_batches(std::array<Batch, 3>{std::move(a), std::move(b), std::move(c)}, caller,
                             static_cast<int>(input.size()));
      });
  return std::get<0>(*ex::sync_wait(std::move(graph)));
}
inline Report run_pipeline(const std::vector<std::string> &input, std::size_t max_records) {
  return run_pipeline(input, max_records, default_score);
}
} // namespace c10_b6
