// =============================================================================
// 练习 J-1：八大经典陷阱重现
//
// 对应文档：12-模块J-陷阱诊断与跨编译器.md  §J-1
//
// 目标：把 C++26 协程工程中最常见的八个生命周期陷阱亲手重现一遍，
//       让 "co_await 不是普通函数调用" 变成肌肉记忆。
//
// 文件结构：
//   - 每个陷阱拆为一个独立的 trapN_xxx() demo 函数；
//   - 每个 demo 都包含 *bad* 版本（注释标注 UB / leak），以及对应的 *good* 修复；
//   - main() 中按陷阱编号顺序串行调用，便于在调试器中单步观察。
//
// 官方参考：
//   - Lewis Baker "Understanding C++ Coroutines"
//     https://lewissbaker.github.io/
//   - Andreas Weis CppCon 2024 "C++ Coroutines: From Basics to Advanced"
//   - Microsoft "Debugging C++ coroutines"
//     https://learn.microsoft.com/en-us/visualstudio/debugger/debug-coroutines
//   - P0912R5 "Merge Coroutines TS into C++20 Working Paper"
//
// 重要约束：
//   - 本文件 **只是骨架**。每个陷阱给出了最小可编译复现 + 修复方案；
//   - 不需要补 TODO 即可编译。要看见崩溃，请取消相应 trapN_run_unsafe() 调用注释。
// =============================================================================

#include <coroutine>
#include <chrono>
#include <exception>
#include <format>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <variant>

// =============================================================================
// 共用：极简 task<T> / generator<T>
// 仅用于演示陷阱，**没有** symmetric transfer，**没有** 调度器线程切换，
// 真实工程请用 stdexec::task 或 cppcoro::task。
// =============================================================================
template <typename T = void>
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

        template <typename U = T>
        void return_value(U&& v) requires (!std::is_void_v<T>) {
            result_.template emplace<1>(std::forward<U>(v));
        }
        void unhandled_exception() {
            result_.template emplace<2>(std::current_exception());
        }
    };

    std::coroutine_handle<promise_type> h_{};
    explicit task(std::coroutine_handle<promise_type> h) : h_(h) {}
    task(task&& o) noexcept : h_(std::exchange(o.h_, {})) {}
    task& operator=(task&&) = delete;
    ~task() { if (h_) h_.destroy(); }

    // 极简 awaiter：把调用者保存为 continuation，恢复 callee
    bool await_ready() const noexcept { return false; }
    std::coroutine_handle<> await_suspend(std::coroutine_handle<> caller) {
        h_.promise().continuation_ = caller;
        return h_;
    }
    T await_resume() requires (!std::is_void_v<T>) {
        auto& r = h_.promise().result_;
        if (r.index() == 2) std::rethrow_exception(std::get<2>(r));
        return std::move(std::get<1>(r));
    }
};

template <>
struct task<void> {
    struct promise_type {
        std::exception_ptr error_{};
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

        void return_void() noexcept {}
        void unhandled_exception() { error_ = std::current_exception(); }
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
    void await_resume() {
        if (h_.promise().error_) std::rethrow_exception(h_.promise().error_);
    }

    // 用最朴素的方式把顶层 task 跑完
    void blocking_run() {
        // 最顶层用一个永不挂起的"空 continuation"——这里直接 resume 一遍
        // 由于本骨架没有线程切换、没有挂起点，resume 会一路跑到 final_suspend
        h_.resume();
        if (h_.promise().error_) std::rethrow_exception(h_.promise().error_);
    }
};

// 总是立刻完成的占位 awaitable —— 用于演示陷阱，不真挂起
struct ready_awaitable {
    bool await_ready() noexcept { return true; }
    void await_suspend(std::coroutine_handle<>) noexcept {}
    void await_resume() noexcept {}
};

inline ready_awaitable some_async_op() noexcept { return {}; }

// =============================================================================
// 陷阱 1：lambda capture by reference 跨 co_await
// =============================================================================
//
// 根本原因：
//   lambda 按引用捕获 *协程外部* 的栈变量。当协程挂起、外部函数返回后，
//   被捕获的引用悬空。恢复时再调用 lambda 即 UB。
//
// 修复：按值捕获 / shared_ptr 延寿。
// -----------------------------------------------------------------------------

namespace trap1 {

// BAD：按引用捕获外部 int&
inline task<void> bad_inner_(int& value) {
    auto lambda = [&value]() { return value; };
    co_await some_async_op();
    // 真实异步路径下，此处 value 可能已悬空 —— 仅注释，不实际访问
    // std::cout << "[trap1] bad lambda() = " << lambda() << "\n";
    (void)lambda;
    co_return;
}

inline task<void> bad_outer_() {
    int value = 42;                        // 在 caller 栈帧上
    auto t = bad_inner_(value);
    co_await std::move(t);
    co_return;
}

// GOOD：按值捕获 / shared_ptr
inline task<void> good_() {
    auto value = std::make_shared<int>(42);
    auto lambda = [value]() { return *value; };
    co_await some_async_op();
    std::cout << "  [trap1][good] lambda() = " << lambda() << "\n";
    co_return;
}

inline void run() {
    std::cout << "[trap1] lambda capture by reference\n";
    bad_outer_().blocking_run();
    good_().blocking_run();
}

} // namespace trap1

// =============================================================================
// 陷阱 2：临时量在 co_await 表达式中析构
// =============================================================================
//
// 根本原因：
//   co_await expr 中，expr 是完整表达式。std::string("hello").c_str() 这样的
//   临时量，c_str() 指针在 awaitable 构造时就拿到了，而 std::string 临时量
//   在分号处即被析构。awaitable 内部存的指针就成了悬空指针。
// -----------------------------------------------------------------------------

namespace trap2 {

struct ptr_awaitable {
    const char* p_;
    explicit ptr_awaitable(const char* p) : p_(p) {}
    bool await_ready() noexcept { return true; }
    void await_suspend(std::coroutine_handle<>) noexcept {}
    // 真实路径上 await_resume 会读取 p_ —— 此处仅打印长度示意
    std::size_t await_resume() noexcept { return p_ ? std::char_traits<char>::length(p_) : 0; }
};

// BAD: c_str() 来自即将析构的临时量
inline task<void> bad_() {
    // 取消下面这行注释即可在某些编译器/优化级别上看到悬空读取：
    // co_await ptr_awaitable(std::string("hello").c_str());
    co_await ready_awaitable{}; // 占位，避免 UB 实际触发
    co_return;
}

// GOOD: 把 std::string 提升为协程帧内的局部变量
inline task<void> good_() {
    std::string s("hello");
    auto n = co_await ptr_awaitable(s.c_str());
    std::cout << "  [trap2][good] len = " << n << "\n";
    co_return;
}

inline void run() {
    std::cout << "[trap2] temporary destroyed inside co_await expression\n";
    bad_().blocking_run();
    good_().blocking_run();
}

} // namespace trap2

// =============================================================================
// 陷阱 3：co_await 期间 lock_guard 跨挂起点
// =============================================================================
//
// 根本原因：
//   lock_guard 析构（解锁）必须在加锁的同一线程。co_await 挂起后恢复线程
//   可能不同，导致跨线程解锁 → UB。单线程测试通常不暴露。
// -----------------------------------------------------------------------------

namespace trap3 {

inline std::mutex g_mtx;
inline int        g_state{0};

// BAD：lock_guard 跨越 co_await
inline task<void> bad_() {
    {
        std::lock_guard<std::mutex> lk(g_mtx);
        co_await some_async_op();         // ← 挂起点跨过锁
        ++g_state;
        // 解锁可能发生在另一个线程
    }
    co_return;
}

// GOOD：先释放锁，再挂起
inline task<void> good_() {
    int snapshot;
    {
        std::unique_lock<std::mutex> lk(g_mtx);
        snapshot = g_state;
        lk.unlock();                      // ← 显式释放
    }
    co_await some_async_op();             // 挂起期间不持锁
    std::cout << "  [trap3][good] snapshot=" << snapshot << "\n";
    co_return;
}

inline void run() {
    std::cout << "[trap3] lock_guard crosses co_await\n";
    bad_().blocking_run();
    good_().blocking_run();
}

} // namespace trap3

// =============================================================================
// 陷阱 4：initial_suspend 抛异常的 frame 泄漏
// =============================================================================
//
// 根本原因：
//   编译器先分配 frame 再调用 initial_suspend()。若它抛异常，
//   get_return_object() 尚未返回 task，handle 没人持有 → 帧泄漏。
//   MSVC 17.10+/Clang 17+/GCC 14+ 已能在该路径上调用 promise 析构与
//   operator delete，但具体保证因 promise 是否完成构造而异。
//   工程上仍应保证 initial_suspend 不抛异常。
// -----------------------------------------------------------------------------

namespace trap4 {

struct leaking_task {
    struct promise_type {
        leaking_task get_return_object() { return {}; }
        // BAD: initial_suspend 抛异常
        std::suspend_always initial_suspend() {
            // throw std::runtime_error("oops");  // ← 取消注释后观察行为
            return {};
        }
        std::suspend_always final_suspend() noexcept { return {}; }
        void return_void() noexcept {}
        void unhandled_exception() {}
    };
};

inline leaking_task bad_() { co_return; }

// GOOD: initial_suspend 始终 noexcept
struct safe_task {
    struct promise_type {
        safe_task get_return_object() { return {}; }
        std::suspend_always initial_suspend() noexcept { return {}; } // ← noexcept
        std::suspend_always final_suspend() noexcept { return {}; }
        void return_void() noexcept {}
        void unhandled_exception() noexcept {}
    };
};

inline safe_task good_() { co_return; }

inline void run() {
    std::cout << "[trap4] initial_suspend throws -> frame leak\n";
    (void)bad_();
    (void)good_();
}

} // namespace trap4

// =============================================================================
// 陷阱 5：detached coroutine 越过创建者生命周期
// =============================================================================
//
// 根本原因：
//   返回 task<> 后既不 co_await 也不交给 async_scope，task 析构会 destroy
//   未运行完的协程帧；如果协程内部捕获了创建者栈引用，UB 在前。
//
// 修复：永远把协程交给 async_scope 或 sync_wait 拥有；不要 detach。
// -----------------------------------------------------------------------------

namespace trap5 {

inline task<void> fire_and_forget_(int* maybe_dangling) {
    co_await some_async_op();
    // *maybe_dangling 可能已悬空
    (void)maybe_dangling;
    co_return;
}

// BAD：把 task 移交给另一线程 detach，caller 立即返回 → local 已悬空，
//      detached 线程上的协程仍可能去读 *maybe_dangling。
inline void bad_() {
    int local = 42;
    std::thread([t = fire_and_forget_(&local)]() mutable {
        // detached 协程在 caller 死后还跑：把它跑完以演示帧仍存活但 local 已死
        t.blocking_run();
    }).detach();
    // 此处 caller 立即返回，local 出作用域；detached 线程读 *maybe_dangling 即 UB
}

// GOOD：交给 async_scope 等价物（此骨架用同步 blocking_run 演示）
inline void good_() {
    int local = 42;
    auto t = fire_and_forget_(&local);
    t.blocking_run();                      // 显式等待完成
}

inline void run() {
    std::cout << "[trap5] detached coroutine outlives creator\n";
    bad_();
    good_();
}

} // namespace trap5

// =============================================================================
// 陷阱 6：generator 返回引用的 dangling
// =============================================================================
//
// 根本原因：
//   generator<T&> 的 co_yield 会把引用透传给消费者；如果 yield 的是临时量，
//   临时量在 co_yield 表达式结束时析构，引用立即悬空。
//   标准 std::generator 的 promise 默认拷贝/移动 yield 值，因此临时量是安全的；
//   但自写 generator 如果只存引用，就会爆雷。
// -----------------------------------------------------------------------------

namespace trap6 {

// 自写极简 generator —— promise 仅保存指针 (BAD pattern)
template <typename T>
struct ref_generator {
    struct promise_type {
        const T* current_{};
        ref_generator get_return_object() {
            return ref_generator{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        std::suspend_always yield_value(const T& v) noexcept {
            current_ = &v;             // ← 仅存引用，临时量很容易悬空
            return {};
        }
        void return_void() noexcept {}
        void unhandled_exception() {}
    };
    std::coroutine_handle<promise_type> h_{};
    explicit ref_generator(std::coroutine_handle<promise_type> h) : h_(h) {}
    ~ref_generator() { if (h_) h_.destroy(); }
};

// BAD：yield 临时量 → 引用悬空
inline ref_generator<int> bad_() {
    co_yield std::max(1, 2);              // ← prvalue 临时量，地址即将失效
    co_return;
}

// GOOD：用按值返回（与 std::generator 一致），或先拷贝到帧内变量
inline ref_generator<int> good_() {
    int max_val = std::max(1, 2);
    co_yield max_val;                     // ← 帧内变量，安全
    co_return;
}

inline void run() {
    std::cout << "[trap6] generator yielding reference to temporary\n";
    auto a = bad_();  (void)a;
    auto b = good_(); (void)b;
}

} // namespace trap6

// =============================================================================
// 陷阱 7：promise destructor 抛异常 → std::terminate
// =============================================================================
//
// 根本原因：
//   coroutine_handle::destroy() 会调用 promise 的析构。析构中抛出异常 →
//   std::terminate（C++ 析构默认 noexcept）。
// -----------------------------------------------------------------------------

namespace trap7 {

struct bad_task {
    struct promise_type {
        bool has_error{false};
        bad_task get_return_object() { return {}; }
        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void return_void() noexcept {}
        void unhandled_exception() {}
        // BAD: 析构未声明 noexcept 且可能 throw → terminate
        ~promise_type() {
            // if (has_error) throw std::runtime_error("cleanup"); // 取消注释 -> terminate
        }
    };
};

struct good_task {
    struct promise_type {
        good_task get_return_object() { return {}; }
        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void return_void() noexcept {}
        void unhandled_exception() noexcept {}
        // GOOD: 显式 noexcept，所有清理逻辑必须吞下异常
        ~promise_type() noexcept {
            // log error, but do not throw
        }
    };
};

inline bad_task  bad_()  { co_return; }
inline good_task good_() { co_return; }

inline void run() {
    std::cout << "[trap7] promise destructor throws -> terminate\n";
    (void)bad_();
    (void)good_();
}

} // namespace trap7

// =============================================================================
// 陷阱 8：自写 generator 的 co_yield 临时量引用
// =============================================================================
//
// 与陷阱 6 同源但更细。区别：标准 std::generator 默认拷贝 yield 值，
// 因此 co_yield "hello"s 在标准版里是安全的；如果你写 generator 时
// promise::yield_value 只存指针/引用，就会出问题。
// -----------------------------------------------------------------------------

namespace trap8 {

// 自写 string_view generator —— BAD：只存引用
struct sv_generator {
    struct promise_type {
        std::string_view current_{};      // ← 引用语义
        sv_generator get_return_object() {
            return sv_generator{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        std::suspend_always yield_value(std::string_view sv) noexcept {
            current_ = sv;                // ← 字符底层 buffer 可能临时
            return {};
        }
        void return_void() noexcept {}
        void unhandled_exception() {}
    };
    std::coroutine_handle<promise_type> h_{};
    explicit sv_generator(std::coroutine_handle<promise_type> h) : h_(h) {}
    ~sv_generator() { if (h_) h_.destroy(); }
};

inline sv_generator bad_() {
    using namespace std::string_literals;
    co_yield "hello"s;                    // ← std::string 临时量
    co_yield std::format("v{}", 1);       // ← std::string 临时量
    co_return;
}

// GOOD（方案 A）：generator 改为按值返回 std::string
struct str_generator {
    struct promise_type {
        std::string current_{};           // ← 拷贝/移动到 frame
        str_generator get_return_object() {
            return str_generator{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        std::suspend_always yield_value(std::string s) noexcept {
            current_ = std::move(s);
            return {};
        }
        void return_void() noexcept {}
        void unhandled_exception() {}
    };
    std::coroutine_handle<promise_type> h_{};
    explicit str_generator(std::coroutine_handle<promise_type> h) : h_(h) {}
    ~str_generator() { if (h_) h_.destroy(); }
};

inline str_generator good_() {
    using namespace std::string_literals;
    co_yield "hello"s;                    // ← 按值进帧，安全
    co_yield std::format("v{}", 1);
    co_return;
}

inline void run() {
    std::cout << "[trap8] custom generator stores reference to temporary\n";
    auto a = bad_();  (void)a;
    auto b = good_(); (void)b;
}

} // namespace trap8

// =============================================================================
// main：按编号顺序串行调用所有陷阱 demo
// =============================================================================
int main() {
    std::cout << "===== Exercise J-1: Eight Coroutine Pitfalls =====\n";
    trap1::run();
    trap2::run();
    trap3::run();
    trap4::run();
    trap5::run();
    trap6::run();
    trap7::run();
    trap8::run();
    std::cout << "===== Done =====\n";
    return 0;
}
