// =============================================================================
// 练习 J-3：协程调试与 tracing
//
// 对应文档：12-模块J-陷阱诊断与跨编译器.md  §J-3
//
// 目标：实现一个 traced_awaitable<Inner> 包装器，在 await_suspend / await_resume
//       自动打 trace；把它应用到 3 个 co_await 点，观察挂起/恢复时序、协程帧地址、
//       线程 ID。建立"协程调试不是玄学"的信心。
//
// 官方参考：
//   - Microsoft "Debugging C++ coroutines"
//     https://learn.microsoft.com/en-us/visualstudio/debugger/debug-coroutines
//   - GDB 14+ "info coroutines"
//     https://sourceware.org/gdb/onlinedocs/gdb/Coroutines.html
//   - folly/experimental/coro tracing 基础设施
//   - Andreas Weis CppCon 2024 "How to Debug C++ Coroutines"
// =============================================================================

#include <atomic>
#include <chrono>
#include <coroutine>
#include <cstdio>
#include <exception>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <utility>
#include <variant>

// =============================================================================
// 共用：极简 task<T>
// =============================================================================
template <typename T>
struct task {
    struct promise_type {
        std::variant<std::monostate, T, std::exception_ptr> result_{};
        std::coroutine_handle<> continuation_{};

        task get_return_object() {
            return task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        std::suspend_always initial_suspend() noexcept { return {}; }

        struct final_awaiter {
            bool await_ready() noexcept { return false; }
            std::coroutine_handle<> await_suspend(
                std::coroutine_handle<promise_type> h) noexcept {
                if (auto cont = h.promise().continuation_) return cont;
                return std::noop_coroutine();
            }
            void await_resume() noexcept {}
        };
        final_awaiter final_suspend() noexcept { return {}; }

        template <typename U>
        void return_value(U&& v) { result_.template emplace<1>(std::forward<U>(v)); }
        void unhandled_exception() {
            result_.template emplace<2>(std::current_exception());
        }
    };

    std::coroutine_handle<promise_type> h_{};
    explicit task(std::coroutine_handle<promise_type> h) : h_(h) {}
    task(task&& o) noexcept : h_(std::exchange(o.h_, {})) {}
    ~task() { if (h_) h_.destroy(); }

    bool await_ready() const noexcept { return false; }
    std::coroutine_handle<> await_suspend(std::coroutine_handle<> caller) {
        h_.promise().continuation_ = caller;
        return h_;
    }
    T await_resume() {
        auto& r = h_.promise().result_;
        if (r.index() == 2) std::rethrow_exception(std::get<2>(r));
        return std::move(std::get<1>(r));
    }

    T blocking_run() {
        h_.resume();
        return await_resume();
    }
};

// =============================================================================
// trace 日志：时间戳 + 事件 + awaitable 名 + 帧地址 + 线程 ID
// 使用 std::cerr 与 mutex 串行化避免日志交错。
//
// 真实生产环境请换为 spdlog / fmt::format / OutputDebugStringA。
// =============================================================================
namespace trace_log {

inline std::mutex& mtx() {
    static std::mutex m;
    return m;
}

inline void log(std::string_view event,
                std::string_view name,
                void* frame_addr) {
    using namespace std::chrono;
    auto now = steady_clock::now().time_since_epoch();
    auto us  = duration_cast<microseconds>(now).count();

    std::ostringstream tid;
    tid << std::this_thread::get_id();

    std::lock_guard<std::mutex> lk(mtx());
    std::cerr << '[' << us << "us]"
              << " [tid=" << tid.str() << ']'
              << ' ' << event
              << ' ' << name
              << " frame=" << frame_addr
              << '\n';
}

} // namespace trace_log

// =============================================================================
// traced_awaitable<Inner>：在 await_suspend / await_resume 自动打 trace
//
// 设计：
//   - 完美转发原始 awaitable；
//   - frame_addr_ 在 await_suspend 时记录，便于在调试器中以帧地址定位协程实例；
//   - 不在 await_resume 中做大量 I/O —— 真实生产中关键路径只记录关键事件。
// =============================================================================
template <typename Inner>
struct traced_awaitable {
    Inner       inner_;
    const char* name_;
    void*       frame_addr_{nullptr};

    template <typename I>
    explicit traced_awaitable(I&& inner, const char* name)
        : inner_(std::forward<I>(inner)), name_(name) {}

    bool await_ready() noexcept(noexcept(inner_.await_ready())) {
        return inner_.await_ready();
    }

    auto await_suspend(std::coroutine_handle<> h) {
        frame_addr_ = h.address();
        trace_log::log("SUSPEND", name_, frame_addr_);
        return inner_.await_suspend(h);
    }

    decltype(auto) await_resume() {
        trace_log::log("RESUME", name_, frame_addr_);
        return inner_.await_resume();
    }
};

// 工厂：避免每次手写模板参数
template <typename Inner>
auto traced(Inner&& inner, const char* name) {
    return traced_awaitable<std::decay_t<Inner>>(std::forward<Inner>(inner), name);
}

// =============================================================================
// 占位 awaitable：always-ready，用于演示 trace 接入点
// 真实工程中 Inner 通常是 timer / future / channel 的 awaitable
// =============================================================================
struct ready_int {
    int v;
    bool await_ready() noexcept { return true; }
    void await_suspend(std::coroutine_handle<>) noexcept {}
    int  await_resume() noexcept { return v; }
};

// =============================================================================
// 全局协程注册表（进阶任务参考实现）
//
// key   = 帧地址
// value = 协程名 + 创建时间 + 状态
// =============================================================================
struct CoroutineInfo {
    std::string                                 name;
    std::chrono::steady_clock::time_point       created_at;
    std::string                                 state; // "running" / "suspended" / "destroyed"
};

class CoroutineRegistry {
    mutable std::mutex                            mtx_;
    std::unordered_map<void*, CoroutineInfo>      map_;
public:
    void register_(void* frame, std::string name) {
        std::lock_guard<std::mutex> lk(mtx_);
        map_[frame] = {std::move(name), std::chrono::steady_clock::now(), "running"};
    }
    void mark_state(void* frame, std::string state) {
        std::lock_guard<std::mutex> lk(mtx_);
        if (auto it = map_.find(frame); it != map_.end())
            it->second.state = std::move(state);
    }
    void unregister_(void* frame) {
        std::lock_guard<std::mutex> lk(mtx_);
        map_.erase(frame);
    }
    void dump() const {
        std::lock_guard<std::mutex> lk(mtx_);
        std::cerr << "----- CoroutineRegistry dump (" << map_.size() << " entries) -----\n";
        for (auto& [frame, info] : map_) {
            std::cerr << "  frame=" << frame
                      << " name=" << info.name
                      << " state=" << info.state << '\n';
        }
    }
};

inline CoroutineRegistry& registry() {
    static CoroutineRegistry r;
    return r;
}

// =============================================================================
// 演示协程：3 个嵌套 + 3 个 co_await 点
//   task_c() -> task_b() -> task_a()
// =============================================================================
inline task<int> task_a() {
    int v = co_await traced(ready_int{1}, "a.step1");
    int w = co_await traced(ready_int{2}, "a.step2");
    co_return v + w;
}

inline task<int> task_b() {
    int x = co_await traced(task_a(), "b.calls_a");
    int y = co_await traced(ready_int{10}, "b.step2");
    co_return x + y;
}

inline task<int> task_c() {
    int z = co_await traced(task_b(), "c.calls_b");
    co_return z * 2;
}

// =============================================================================
// 进阶：traced_task<T> —— 在 promise 的 initial/final_suspend 也打 trace
// 这里给出接口形状，实现细节留给你填充（也可以照 task 抄一份）
// =============================================================================
template <typename T>
struct traced_task {
    // TODO[进阶]：复制 task<T> 后在 initial_suspend / final_suspend 中调用
    //           trace_log::log("INIT" / "FINAL", ...) 即可。
    //           本骨架不展开，留作 §J-3 进阶任务。
};

int main() {
    std::cout << "===== Exercise J-3: Coroutine Tracing =====\n";

    // --- 主任务：跑一遍嵌套协程，观察 SUSPEND / RESUME trace 顺序 ---
    auto t = task_c();
    int  result = t.blocking_run();
    std::cout << "[main] task_c result = " << result << " (expect 26)\n";

    // --- 进阶：用注册表查询当前活跃协程 ---
    registry().dump();

    // --- trace 日志使用建议 ---
    // 1. 把 traced(Inner, name) 套在所有关键 co_await 点上；
    // 2. 把日志接到统一的 sink（cerr / file / OTLP）；
    // 3. 在 Release 构建中通过宏关闭：
    //      #ifdef NDEBUG
    //        #define traced(x, n) (x)
    //      #endif
    // 4. 配合 MSVC Parallel Stacks（Tasks 视图）/ GDB `info coroutines`，
    //    可以同时看到"物理线程栈"和"协程逻辑链"。

    std::cout << "===== Done =====\n";
    return 0;
}
