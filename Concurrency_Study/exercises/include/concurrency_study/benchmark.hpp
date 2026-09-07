#ifndef CONCURRENCY_STUDY_BENCHMARK_HPP
#define CONCURRENCY_STUDY_BENCHMARK_HPP

#include <charconv>
#include <chrono>
#include <cstddef>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace cs::bench {

// Each executable owns its option set. Reading consumes an option; finish()
// catches misspellings instead of silently benchmarking the default case.
class arguments {
public:
    arguments(int argc, char** argv) {
        if ((argc - 1) % 2 != 0) throw std::invalid_argument("options use --name value pairs");
        for (int i = 1; i < argc; i += 2) {
            std::string key = argv[i];
            if (!key.starts_with("--") || !values_.emplace(key, argv[i + 1]).second)
                throw std::invalid_argument("invalid or duplicate option: " + key);
        }
    }

    std::string text(std::string_view name, std::string fallback) {
        const auto found = values_.find(std::string{name});
        if (found == values_.end()) return fallback;
        std::string value = std::move(found->second);
        values_.erase(found);
        return value;
    }

    std::size_t number(std::string_view name, std::size_t fallback) {
        const auto value = text(name, std::to_string(fallback));
        std::size_t parsed = 0;
        const auto result = std::from_chars(value.data(), value.data() + value.size(), parsed);
        if (result.ec != std::errc{} || result.ptr != value.data() + value.size())
            throw std::invalid_argument("invalid nonnegative integer for " + std::string{name});
        return parsed;
    }

    void finish() const {
        if (!values_.empty()) throw std::invalid_argument("unknown option: " + values_.begin()->first);
    }

private:
    std::map<std::string, std::string> values_;
};

template<class F>
double measure_ms(F&& operation) {
    const auto begin = std::chrono::steady_clock::now();
    std::invoke(std::forward<F>(operation));
    return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - begin).count();
}

inline std::string csv(std::string_view value) {
    std::string quoted = "\"";
    for (const char c : value) {
        if (c == '"') quoted += '"';
        quoted += c;
    }
    quoted += '"';
    return quoted;
}

// threads == 0 means an implementation-managed thread count was not measured,
// not that zero execution threads performed the work. Explain it in details.
inline void emit_row(std::string_view suite, std::string_view variant,
                     std::size_t size, std::size_t threads, double milliseconds,
                     std::size_t completed, std::string_view details = {}) {
    static bool header = false; // Call only after workers have joined.
    if (!header) {
        std::cout << "suite,variant,size,threads,milliseconds,completed,details\n";
        header = true;
    }
    std::cout << csv(suite) << ',' << csv(variant) << ',' << size << ',' << threads
              << ',' << std::setprecision(12) << milliseconds << ',' << completed
              << ',' << csv(details) << '\n';
}

} // namespace cs::bench
#endif
