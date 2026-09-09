#ifndef CONCURRENCY_STUDY_LOG_HPP
#define CONCURRENCY_STUDY_LOG_HPP

#include <chrono>
#include <iostream>
#include <sstream>
#include <string_view>
#include <syncstream>
#include <thread>
#include <utility>

namespace cs {

// Historical name retained: this is first use, not actual process startup.
inline const std::chrono::steady_clock::time_point& program_start() {
    static const auto first_use = std::chrono::steady_clock::now();
    return first_use;
}

inline long long now_ms() {
    const auto& start = program_start();
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start).count();
}

// All cooperating writes must use osyncstream. Direct cout writes are outside
// this line-integrity guarantee. Logging perturbs scheduling and may add
// synchronization; keep it outside memory-model litmus and benchmark windows.
inline void log(std::string_view message) {
    std::osyncstream{std::cout} << "[ +" << now_ms() << "ms | tid "
        << std::this_thread::get_id() << " ] " << message << '\n' << std::flush_emit;
}

inline void println(std::string_view message) {
    std::osyncstream{std::cout} << message << '\n' << std::flush_emit;
}

template<class... Args>
inline void logf(Args&&... args) {
    std::ostringstream line;
    (line << ... << std::forward<Args>(args));
    log(line.str());
}

} // namespace cs
#endif
