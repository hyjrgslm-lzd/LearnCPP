#pragma once
#include <stdexec/execution.hpp>
#include <exec/static_thread_pool.hpp>
#include <array>
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
struct Row {
  std::string raw;
  std::string name;
  int value{};
  bool ok{};
  double score{};
  std::vector<StageRecord> stages;
};
struct Chunk {
  int id{};
  std::vector<Row> rows;
};
inline bool parse_decimal(std::string_view s, int &out) {
  if (s.empty())
    return false;
  long long v = 0;
  for (char c : s) {
    if (c < '0' || c > '9')
      return false;
    v = v * 10 + (c - '0');
    if (v > 1000000)
      return false;
  }
  out = static_cast<int>(v);
  return true;
}
inline bool parse_name(std::string_view s) {
  if (s.empty())
    return false;
  auto alpha = [](unsigned char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_';
  };
  auto alnum = [&](unsigned char c) { return alpha(c) || (c >= '0' && c <= '9'); };
  if (!alpha(static_cast<unsigned char>(s[0])))
    return false;
  for (char c : s.substr(1))
    if (!alnum(static_cast<unsigned char>(c)))
      return false;
  return true;
}
inline Row parse_row(std::string raw, int branch) {
  Row row{};
  row.raw = std::move(raw);
  row.stages.push_back({"parse", row.raw, branch, std::this_thread::get_id()});
  auto text = std::string_view(row.raw);
  auto comma = text.find(',');
  if (comma == std::string_view::npos || text.find(',', comma + 1) != std::string_view::npos)
    return row;
  int value{};
  auto lhs = text.substr(0, comma);
  auto rhs = text.substr(comma + 1);
  if (!parse_name(lhs) || !parse_decimal(rhs, value))
    return row;
  row.name = std::string(lhs);
  row.value = value;
  row.ok = true;
  return row;
}
inline Chunk parse_chunk(std::vector<std::string> rows, int id) {
  Chunk c{};
  c.id = id;
  for (auto &row : rows)
    c.rows.push_back(parse_row(std::move(row), id));
  return c;
}
inline Chunk enrich_chunk(Chunk c, EnrichHook &hook) {
  for (auto &row : c.rows)
    if (row.ok) {
      row.score = hook(row.name, row.value);
      row.stages.push_back({"enrich", row.name, c.id, std::this_thread::get_id()});
    }
  return c;
}
inline double default_score(std::string_view name, int value) {
  return value * 1.5 + static_cast<double>(name.size());
}
inline Report run_pipeline(const std::vector<std::string> &input, std::size_t max_records,
                           EnrichHook hook) {
  if (input.size() > max_records)
    throw std::invalid_argument("too many records");
  std::array<std::vector<std::string>, 3> chunks;
  for (std::size_t i = 0; i < input.size(); ++i)
    chunks[i % 3].push_back(input[i]);
  auto caller = std::this_thread::get_id();
  exec::static_thread_pool parser(1), enricher(1), merger(1);
  auto ps = parser.get_scheduler();
  auto es = enricher.get_scheduler();
  auto ms = merger.get_scheduler();
  auto make = [&](int id) {
    return ex::starts_on(
        ps, ex::just(std::move(chunks[id]), id) | ex::then(parse_chunk) | ex::continues_on(es) |
                ex::then([&](Chunk c) { return enrich_chunk(std::move(c), hook); }));
  };
  auto got = ex::sync_wait(ex::when_all(make(0), make(1), make(2)) | ex::continues_on(ms) |
                           ex::then([=](Chunk a, Chunk b, Chunk c) mutable {
                             Report r{};
                             r.total = static_cast<int>(input.size());
                             r.caller_thread = caller;
                             r.merge_thread = std::this_thread::get_id();
                             r.completed_batches = 3;
                             for (auto *chunk : {&a, &b, &c})
                               for (auto &row : chunk->rows) {
                                 for (auto &stage : row.stages)
                                   r.stages.push_back(std::move(stage));
                                 if (row.ok) {
                                   ++r.valid_count;
                                   r.total_score += row.score;
                                 } else {
                                   ++r.invalid_count;
                                 }
                               }
                             return r;
                           }));
  return std::get<0>(*got);
}
inline Report run_pipeline(const std::vector<std::string> &input, std::size_t max_records) {
  return run_pipeline(input, max_records, default_score);
}
} // namespace c10_b6
