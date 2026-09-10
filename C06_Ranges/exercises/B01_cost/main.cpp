#include <check.hpp>
#include <log_pipeline.hpp>

#include <algorithm>
#include <chrono>
#include <charconv>
#include <cstdint>
#include <iostream>
#include <map>
#include <optional>
#include <ranges>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

using Clock = std::chrono::steady_clock;

struct PipelineInput {
    std::vector<std::string> storage;
    std::vector<std::string_view> views;
    std::vector<capstone1::LogRecord> expected_records;
};

struct PipelineResult {
    std::uint64_t checksum{};
    std::uint64_t oracle_checksum{};
    int valid_records{};
    int oracle_valid_records{};
    int error_records{};
    int oracle_error_records{};
    int parse_calls{};
    int optional_slots{};
    int records_materialized{};
    long long elapsed_ns{};
};

struct IndexRecord {
    int key{};
    int payload{};
};

struct IndexStats {
    std::uint64_t comparisons{};
    std::uint64_t hash_calls{};
    std::uint64_t equal_calls{};
};

struct IndexResult {
    std::uint64_t checksum{};
    std::uint64_t oracle_checksum{};
    int matches{};
    int oracle_matches{};
    long long build_ns{};
    long long lookup_ns{};
    IndexStats build;
    IndexStats lookup;
};

struct Scenario {
    int n{};
    int rate{};
};

std::uint64_t mix(std::uint64_t value) {
    value ^= value >> 30;
    value *= 0xbf58476d1ce4e5b9ULL;
    value ^= value >> 27;
    value *= 0x94d049bb133111ebULL;
    value ^= value >> 31;
    return value;
}

std::uint64_t next_random(std::uint64_t& state) {
    state += 0x9e3779b97f4a7c15ULL;
    return mix(state);
}

int parse_int(std::string_view text, std::string_view name) {
    int value{};
    auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (ec != std::errc{} || ptr != text.data() + text.size()) {
        throw std::invalid_argument(std::string(name) + " must be an integer");
    }
    return value;
}

Scenario scenario_from_name(std::string_view name, std::string_view prefix, std::string_view low_suffix, std::string_view high_suffix) {
    if (name.starts_with("small_")) {
        if (name.ends_with(low_suffix)) return {256, 10};
        if (name.ends_with(high_suffix)) return {256, 90};
    }
    if (name.starts_with("large_")) {
        if (name.ends_with(low_suffix)) return {8192, 10};
        if (name.ends_with(high_suffix)) return {8192, 90};
    }
    throw std::invalid_argument(std::string(prefix) + " scenario is unknown");
}

void enforce_limits(int n, int queries) {
    if (n <= 0 || n > 16384) throw std::invalid_argument("n must be in 1..16384");
    if (queries <= 0 || queries > n) throw std::invalid_argument("queries must be in 1..n");
}

PipelineInput make_pipeline_input(int n, int valid_rate, std::uint64_t seed) {
    enforce_limits(n, n);
    PipelineInput input;
    input.storage.reserve(static_cast<std::size_t>(n));
    input.views.reserve(static_cast<std::size_t>(n));
    input.expected_records.reserve(static_cast<std::size_t>(n));
    for (int i = 0; i < n; ++i) {
        const bool valid = static_cast<int>(next_random(seed) % 100) < valid_rate;
        if (!valid) {
            input.storage.push_back(i % 2 == 0 ? "BROKEN_LINE" : "2024-01-01T00:00:00,BADLEVEL,user,bad");
        } else {
            const auto level_text = i % 5 == 0 ? "ERROR" : (i % 3 == 0 ? "WARN" : "INFO");
            const auto level = i % 5 == 0 ? capstone1::Level::error : (i % 3 == 0 ? capstone1::Level::warn : capstone1::Level::info);
            auto timestamp = "2024-01-01T00:" + std::to_string((i / 60) % 60) + ":" + std::to_string(i % 60);
            auto user_id = "user" + std::to_string(static_cast<int>(next_random(seed) % 128));
            auto message = "message" + std::to_string(i);
            input.storage.push_back(timestamp + "," + level_text + "," + user_id + "," + message);
            input.expected_records.push_back({std::move(timestamp), level, std::move(user_id), std::move(message)});
        }
    }
    for (const auto& line : input.storage) {
        input.views.push_back(line);
    }
    return input;
}

std::uint64_t checksum_records(std::span<const capstone1::LogRecord> records) {
    std::uint64_t result = 1469598103934665603ULL;
    auto add_string = [&](std::string_view text) {
        for (char ch : text) {
            result ^= static_cast<unsigned char>(ch);
            result *= 1099511628211ULL;
        }
    };
    for (const auto& record : records) {
        add_string(record.timestamp);
        result ^= static_cast<unsigned>(record.level) + 1U;
        result *= 1099511628211ULL;
        add_string(record.user_id);
        add_string(record.message);
    }
    return result;
}

bool same_records(std::span<const capstone1::LogRecord> left, std::span<const capstone1::LogRecord> right) {
    return std::ranges::equal(left, right, {}, [](const capstone1::LogRecord& record) {
        return std::tie(record.timestamp, record.level, record.user_id, record.message);
    }, [](const capstone1::LogRecord& record) {
        return std::tie(record.timestamp, record.level, record.user_id, record.message);
    });
}

template<bool counted>
PipelineResult run_pipeline_impl(std::string_view variant, std::string_view scenario, int n_override, int rate_override, std::uint64_t seed) {
    auto config = scenario_from_name(scenario, "pipeline", "low_valid", "high_valid");
    if (n_override > 0) config.n = n_override;
    if (rate_override >= 0) config.rate = rate_override;
    if (config.rate < 0 || config.rate > 100) throw std::invalid_argument("valid rate must be in 0..100");
    auto input = make_pipeline_input(config.n, config.rate, seed);

    capstone1::OperationCounts counts;
    std::vector<capstone1::LogRecord> records;
    int optional_slots = 0;
    const auto start = Clock::now();
    if (variant == "loop") {
        if constexpr (counted) records = capstone1::collect_records_loop(input.views, counts);
        else records = capstone1::collect_records_loop(input.views);
    } else if (variant == "materialized") {
        if constexpr (counted) records = capstone1::collect_records_ranges_once(input.views, counts);
        else records = capstone1::collect_records_ranges_once(input.views);
        optional_slots = config.n;
    } else if (variant == "reparse") {
        auto parse_counted = [&counts](std::string_view line) {
            if constexpr (counted) ++counts.parse_attempts;
            return capstone1::parse_line(line);
        };
        auto view = input.views | std::views::transform(parse_counted)
                  | std::views::filter([](const std::optional<capstone1::LogRecord>& record) {
                        return record.has_value();
                    })
                  | std::views::transform([](std::optional<capstone1::LogRecord> record) {
                        return std::move(*record);
                    });
        records = view | std::ranges::to<std::vector<capstone1::LogRecord>>();
    } else {
        throw std::invalid_argument("pipeline variant is unknown");
    }
    const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - start).count();
    const int errors = static_cast<int>(std::ranges::count_if(records, [](capstone1::Level level) {
        return level == capstone1::Level::error;
    }, &capstone1::LogRecord::level));
    const int oracle_errors = static_cast<int>(std::ranges::count_if(input.expected_records, [](capstone1::Level level) {
        return level == capstone1::Level::error;
    }, &capstone1::LogRecord::level));
    if (!same_records(records, input.expected_records)) {
        throw std::logic_error("pipeline result differs from independent oracle");
    }
    return {checksum_records(records), checksum_records(input.expected_records),
            static_cast<int>(records.size()), static_cast<int>(input.expected_records.size()), errors, oracle_errors, counts.parse_attempts,
            optional_slots, static_cast<int>(records.size()), elapsed};
}

PipelineResult run_pipeline(std::string_view variant, std::string_view scenario, std::string_view instrument,
                            int n_override, int rate_override, std::uint64_t seed) {
    if (instrument == "counted") return run_pipeline_impl<true>(variant, scenario, n_override, rate_override, seed);
    if (instrument == "timed") return run_pipeline_impl<false>(variant, scenario, n_override, rate_override, seed);
    throw std::invalid_argument("instrument must be counted or timed");
}

std::vector<IndexRecord> make_records(int n, std::uint64_t seed) {
    enforce_limits(n, n);
    std::vector<IndexRecord> records;
    records.reserve(static_cast<std::size_t>(n));
    for (int i = 0; i < n; ++i) {
        records.push_back({i * 2, static_cast<int>(next_random(seed) % 100000)});
    }
    return records;
}

std::vector<int> make_queries(int n, int hit_rate, std::uint64_t seed) {
    enforce_limits(n, n);
    if (hit_rate < 0 || hit_rate > 100) throw std::invalid_argument("hit rate must be in 0..100");
    std::vector<int> queries;
    queries.reserve(static_cast<std::size_t>(n));
    for (int i = 0; i < n; ++i) {
        const bool hit = static_cast<int>(next_random(seed) % 100) < hit_rate;
        const int key_index = static_cast<int>(next_random(seed) % static_cast<std::uint64_t>(n));
        queries.push_back(hit ? key_index * 2 : key_index * 2 + 1);
    }
    return queries;
}

struct CountedLess {
    IndexStats* stats{};
    bool operator()(const IndexRecord& left, const IndexRecord& right) const {
        if (stats) ++stats->comparisons;
        return left.key < right.key;
    }
    bool operator()(const IndexRecord& left, int right) const {
        if (stats) ++stats->comparisons;
        return left.key < right;
    }
    bool operator()(int left, const IndexRecord& right) const {
        if (stats) ++stats->comparisons;
        return left < right.key;
    }
    bool operator()(int left, int right) const {
        if (stats) ++stats->comparisons;
        return left < right;
    }
};

struct CountedMapLess {
    IndexStats** stats{};
    bool operator()(int left, int right) const {
        if (stats && *stats) ++(*stats)->comparisons;
        return left < right;
    }
};

struct CountedHash {
    IndexStats** stats{};
    std::size_t operator()(int value) const {
        if (stats && *stats) ++(*stats)->hash_calls;
        return std::hash<int>{}(value);
    }
};

struct CountedEqual {
    IndexStats** stats{};
    bool operator()(int left, int right) const {
        if (stats && *stats) ++(*stats)->equal_calls;
        return left == right;
    }
};

std::uint64_t update_checksum(std::uint64_t checksum, int key, int payload) {
    return mix(checksum ^ (static_cast<std::uint64_t>(key) << 32) ^ static_cast<unsigned>(payload));
}

IndexResult index_oracle(std::span<const IndexRecord> records, std::span<const int> queries) {
    IndexResult result;
    result.checksum = 1469598103934665603ULL;
    for (int query : queries) {
        for (const auto& record : records) {
            if (record.key == query) {
                ++result.matches;
                result.checksum = update_checksum(result.checksum, record.key, record.payload);
                break;
            }
        }
    }
    return result;
}

IndexResult run_index(std::string_view variant, std::string_view scenario, std::string_view instrument,
                      int n_override, int hit_override, std::uint64_t seed) {
    auto config = scenario_from_name(scenario, "index", "low_hit", "high_hit");
    if (n_override > 0) config.n = n_override;
    if (hit_override >= 0) config.rate = hit_override;
    auto records = make_records(config.n, seed);
    auto queries = make_queries(config.n, config.rate, seed);
    const bool counted = instrument == "counted";
    if (!counted && instrument != "timed") throw std::invalid_argument("instrument must be counted or timed");

    IndexResult result;
    result.checksum = 1469598103934665603ULL;

    if (variant == "linear_vector") {
        std::vector<IndexRecord> index;
        const auto build_start = Clock::now();
        index = records;
        result.build_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - build_start).count();

        const auto lookup_start = Clock::now();
        for (int query : queries) {
            auto found = std::ranges::find_if(index, [&](const IndexRecord& record) {
                if (counted) ++result.lookup.comparisons;
                return record.key == query;
            });
            if (found != index.end()) {
                ++result.matches;
                result.checksum = update_checksum(result.checksum, found->key, found->payload);
            }
        }
        result.lookup_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - lookup_start).count();
    } else if (variant == "sorted_vector") {
        std::vector<IndexRecord> index;
        CountedLess less{counted ? &result.build : nullptr};
        const auto build_start = Clock::now();
        index = records;
        std::ranges::sort(index, less);
        result.build_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - build_start).count();

        CountedLess lookup_less{counted ? &result.lookup : nullptr};
        const auto lookup_start = Clock::now();
        for (int query : queries) {
            auto found = std::ranges::lower_bound(index, query, lookup_less);
            if (found != index.end()) {
                if (counted) ++result.lookup.comparisons;
                if (found->key == query) {
                    ++result.matches;
                    result.checksum = update_checksum(result.checksum, found->key, found->payload);
                }
            }
        }
        result.lookup_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - lookup_start).count();
    } else if (variant == "map") {
        IndexStats* active_stats = counted ? &result.build : nullptr;
        std::map<int, int, CountedMapLess> index{CountedMapLess{counted ? &active_stats : nullptr}};
        const auto build_start = Clock::now();
        for (auto record : records) {
            index.emplace(record.key, record.payload);
        }
        result.build_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - build_start).count();

        active_stats = counted ? &result.lookup : nullptr;
        const auto lookup_start = Clock::now();
        for (int query : queries) {
            auto found = index.find(query);
            if (found != index.end()) {
                ++result.matches;
                result.checksum = update_checksum(result.checksum, found->first, found->second);
            }
        }
        result.lookup_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - lookup_start).count();
    } else if (variant == "unordered_map") {
        IndexStats* active_stats = counted ? &result.build : nullptr;
        std::unordered_map<int, int, CountedHash, CountedEqual> index{
            0, CountedHash{counted ? &active_stats : nullptr}, CountedEqual{counted ? &active_stats : nullptr}};
        const auto build_start = Clock::now();
        index.reserve(records.size());
        for (auto record : records) {
            index.emplace(record.key, record.payload);
        }
        result.build_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - build_start).count();

        active_stats = counted ? &result.lookup : nullptr;
        const auto lookup_start = Clock::now();
        for (int query : queries) {
            auto found = index.find(query);
            if (found != index.end()) {
                ++result.matches;
                result.checksum = update_checksum(result.checksum, found->first, found->second);
            }
        }
        result.lookup_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - lookup_start).count();
    } else {
        throw std::invalid_argument("index variant is unknown");
    }

    auto expected = index_oracle(records, queries);
    result.oracle_checksum = expected.checksum;
    result.oracle_matches = expected.matches;
    if (result.checksum != expected.checksum || result.matches != expected.matches) {
        throw std::logic_error("index result differs from independent oracle");
    }
    return result;
}

void print_json_string(std::string_view value) {
    std::cout << '"';
    for (char ch : value) {
        if (ch == '"' || ch == '\\') std::cout << '\\';
        std::cout << ch;
    }
    std::cout << '"';
}

void print_pipeline_json(std::string_view scenario, std::string_view variant, std::string_view instrument, std::uint64_t seed,
                         int n, int rate, const PipelineResult& result) {
    std::cout << "{\"schema\":1,\"case\":\"pipeline\",\"scenario\":";
    print_json_string(scenario);
    std::cout << ",\"variant\":";
    print_json_string(variant);
    std::cout << ",\"instrument\":";
    print_json_string(instrument);
    std::cout << ",\"seed\":" << seed << ",\"n\":" << n << ",\"rate\":" << rate
              << ",\"checksum\":" << result.checksum
              << ",\"oracle_checksum\":" << result.oracle_checksum
              << ",\"valid_records\":" << result.valid_records
              << ",\"oracle_valid_records\":" << result.oracle_valid_records
              << ",\"error_records\":" << result.error_records
              << ",\"oracle_error_records\":" << result.oracle_error_records
              << ",\"parse_calls\":" << result.parse_calls
              << ",\"optional_slots\":" << result.optional_slots
              << ",\"records_materialized\":" << result.records_materialized
              << ",\"elapsed_ns\":" << result.elapsed_ns << "}\n";
}

void print_index_json(std::string_view scenario, std::string_view variant, std::string_view instrument,
                      std::uint64_t seed, int n, int rate, const IndexResult& result) {
    std::cout << "{\"schema\":1,\"case\":\"index\",\"scenario\":";
    print_json_string(scenario);
    std::cout << ",\"variant\":";
    print_json_string(variant);
    std::cout << ",\"instrument\":";
    print_json_string(instrument);
    std::cout << ",\"seed\":" << seed << ",\"n\":" << n << ",\"rate\":" << rate
              << ",\"checksum\":" << result.checksum
              << ",\"oracle_checksum\":" << result.oracle_checksum
              << ",\"matches\":" << result.matches
              << ",\"oracle_matches\":" << result.oracle_matches
              << ",\"build_ns\":" << result.build_ns
              << ",\"lookup_ns\":" << result.lookup_ns
              << ",\"build_comparisons\":" << result.build.comparisons
              << ",\"lookup_comparisons\":" << result.lookup.comparisons
              << ",\"build_hash_calls\":" << result.build.hash_calls
              << ",\"lookup_hash_calls\":" << result.lookup.hash_calls
              << ",\"build_equal_calls\":" << result.build.equal_calls
              << ",\"lookup_equal_calls\":" << result.lookup.equal_calls << "}\n";
}

void run_check() {
    auto loop = run_pipeline("loop", "small_high_valid", "counted", 256, -1, 100);
    auto materialized = run_pipeline("materialized", "small_high_valid", "counted", 256, -1, 100);
    auto reparse = run_pipeline("reparse", "small_high_valid", "counted", 256, -1, 100);
    check(loop.checksum == materialized.checksum && loop.checksum == reparse.checksum, "pipeline variants keep checksum");
    check(loop.valid_records == materialized.valid_records && loop.valid_records == reparse.valid_records,
          "pipeline variants keep valid record count");
    check(loop.checksum == loop.oracle_checksum && loop.valid_records == loop.oracle_valid_records
          && loop.error_records == loop.oracle_error_records,
          "pipeline compares full result with independent oracle");
    check(loop.parse_calls == 256 && materialized.parse_calls == 256, "single-pass variants parse each line once");
    check(reparse.parse_calls > loop.parse_calls, "reparse variant performs extra parse calls");
    auto timed = run_pipeline("loop", "small_high_valid", "timed", 256, -1, 100);
    check(timed.parse_calls == 0 && timed.checksum == loop.checksum, "timed pipeline keeps instrumentation outside timing path");

    auto linear = run_index("linear_vector", "small_high_hit", "counted", 256, -1, 200);
    auto sorted = run_index("sorted_vector", "small_high_hit", "counted", 256, -1, 200);
    auto tree = run_index("map", "small_high_hit", "counted", 256, -1, 200);
    auto hash = run_index("unordered_map", "small_high_hit", "counted", 256, -1, 200);
    check(linear.checksum == sorted.checksum && linear.checksum == tree.checksum && linear.checksum == hash.checksum,
          "index variants keep checksum");
    check(linear.matches == sorted.matches && linear.matches == tree.matches && linear.matches == hash.matches,
          "index variants keep match count");
    check(linear.checksum == linear.oracle_checksum && linear.matches == linear.oracle_matches,
          "index compares result with independent oracle");
    check(linear.lookup.comparisons > 0 && sorted.lookup.comparisons > 0 && tree.lookup.comparisons > 0,
          "counted index variants record comparisons");
    check(hash.build.hash_calls > 0 && hash.lookup.hash_calls > 0, "counted unordered_map records hash calls");
    std::cout << "B01_cost checks OK\n";
}

} // namespace

int main(int argc, char** argv) {
    try {
        if (argc == 1 || std::string_view(argv[1]) == "--check") {
            run_check();
            return 0;
        }

        std::string_view mode = argv[1];
        if (mode != "--bench-one") throw std::invalid_argument("expected --check or --bench-one");

        std::string_view bench_case;
        std::string_view scenario;
        std::string_view variant;
        std::string_view instrument = "timed";
        std::uint64_t seed = 42;
        int n_override = -1;
        int rate_override = -1;
        for (int i = 2; i < argc; ++i) {
            std::string_view arg = argv[i];
            auto need_value = [&](std::string_view name) -> std::string_view {
                if (++i >= argc) throw std::invalid_argument(std::string(name) + " needs a value");
                return argv[i];
            };
            if (arg == "--case") bench_case = need_value(arg);
            else if (arg == "--scenario") scenario = need_value(arg);
            else if (arg == "--variant") variant = need_value(arg);
            else if (arg == "--instrument") instrument = need_value(arg);
            else if (arg == "--seed") seed = static_cast<std::uint64_t>(parse_int(need_value(arg), arg));
            else if (arg == "--n") n_override = parse_int(need_value(arg), arg);
            else if (arg == "--rate") rate_override = parse_int(need_value(arg), arg);
            else throw std::invalid_argument("unknown argument");
        }
        if (bench_case.empty() || scenario.empty() || variant.empty()) {
            throw std::invalid_argument("--case, --scenario and --variant are required");
        }

        if (bench_case == "pipeline") {
            auto config = scenario_from_name(scenario, "pipeline", "low_valid", "high_valid");
            if (n_override > 0) config.n = n_override;
            if (rate_override >= 0) config.rate = rate_override;
            auto result = run_pipeline(variant, scenario, instrument, n_override, rate_override, seed);
            print_pipeline_json(scenario, variant, instrument, seed, config.n, config.rate, result);
        } else if (bench_case == "index") {
            auto config = scenario_from_name(scenario, "index", "low_hit", "high_hit");
            if (n_override > 0) config.n = n_override;
            if (rate_override >= 0) config.rate = rate_override;
            auto result = run_index(variant, scenario, instrument, n_override, rate_override, seed);
            print_index_json(scenario, variant, instrument, seed, config.n, config.rate, result);
        } else {
            throw std::invalid_argument("case must be pipeline or index");
        }
    } catch (const std::exception& error) {
        std::cerr << "B01_cost error: " << error.what() << '\n';
        return 1;
    }
}
