// =====================================================================
// concurrency_study/log.hpp
//   阶段一起步即用的线程安全日志/计时小工具（header-only，跨平台）。
//
//   为什么需要它：并发程序的输出来自多个线程，若直接用 std::cout <<
//   分多次插入，不同线程的片段会交错（interleave）成乱码。本工具用一把
//   内部互斥锁保证“一条日志原子落地”，并统一带上线程 id 与毫秒级时间戳，
//   方便你在练习里观察事件的先后顺序。
//
//   这不是高性能日志（它本身会串行化所有打印），仅用于学习期观测。
//   真正追求吞吐时，打印本身就是瓶颈——这一点你在做题时应当亲自体会到。
// =====================================================================
#ifndef CONCURRENCY_STUDY_LOG_HPP
#define CONCURRENCY_STUDY_LOG_HPP

#include <chrono>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>

namespace cs {

// 进程启动时刻（用于打印相对时间戳，比绝对挂钟时间更易读）。
inline const std::chrono::steady_clock::time_point& program_start() {
    static const std::chrono::steady_clock::time_point t0 =
        std::chrono::steady_clock::now();
    return t0;
}

// 保护标准输出的全局锁。日志与 println 共用同一把锁。
inline std::mutex& io_mutex() {
    static std::mutex m;
    return m;
}

// 当前线程相对启动点的毫秒数。
inline long long now_ms() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now() - program_start()).count();
}

// 线程安全地打印一行（自动追加换行）。带 [ +时间ms | tid ] 前缀。
inline void log(std::string_view msg) {
    std::ostringstream oss;
    oss << "[ +" << now_ms() << "ms | tid " << std::this_thread::get_id() << " ] "
        << msg << '\n';
    std::lock_guard<std::mutex> lk(io_mutex());
    std::cout << oss.str();
    std::cout.flush();
}

// 线程安全的“原样打印一行”（不加前缀），用于打印表头/分隔线等。
inline void println(std::string_view msg) {
    std::lock_guard<std::mutex> lk(io_mutex());
    std::cout << msg << '\n';
    std::cout.flush();
}

// 变参版本：cs::logf("worker ", id, " done, n=", n);
template <class... Args>
inline void logf(Args&&... args) {
    std::ostringstream oss;
    (oss << ... << args);
    log(oss.str());
}

} // namespace cs

#endif // CONCURRENCY_STUDY_LOG_HPP
