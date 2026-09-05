// =============================================================================
// 练习 J-2：跨编译器 ABI 与符号问题
//
// 对应文档：12-模块J-陷阱诊断与跨编译器.md  §J-2
//
// 目标：同一份最小 task<T> 在 MSVC / Clang / GCC 上各编一遍，对比：
//       - 协程帧大小
//       - HALO 触发情况
//       - 调试信息中 frame 变量可读性
//       - coroutine_handle 跨 DLL 边界传递的崩溃模式
//
// 验收：填完两份对比表，跑过一次跨 DLL 实验。
//
// 官方参考：
//   - Microsoft "Debugging C++ coroutines"
//     https://learn.microsoft.com/en-us/visualstudio/debugger/debug-coroutines
//   - GDB 14+ coroutine support
//     https://sourceware.org/gdb/onlinedocs/gdb/Coroutines.html
//   - Clang `-Rpass=coroutine-elide`
//   - GCC `-fdump-tree-coro` / `-fdump-ipa-coro`
//   - P0912R5 Merge Coroutines TS into C++20 working draft
//
// 编译诊断 flag（CMakeLists 中已注入）：
//   - MSVC : /d1reportSingleClassLayoutminimal_promise /Zi /await:strict
//   - Clang: -Rpass=coroutine-elide -Xclang -fdump-record-layouts
//   - GCC  : -fdump-tree-coro -fdump-ipa-coro
// =============================================================================

#include <coroutine>
#include <cstddef>
#include <cstdio>
#include <exception>
#include <iostream>
#include <optional>
#include <utility>
#include <variant>

// =============================================================================
// 最小但功能完整的 task<T>（约 80 行）：
//   - promise_type 含 initial/final_suspend、return_value、unhandled_exception
//   - final_suspend 走 symmetric transfer
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

    // 顶层 blocking 入口 —— 协程帧地址永不逃逸 → HALO 候选
    T blocking_run() {
        h_.resume();
        return await_resume();
    }
};

// =============================================================================
// 实验 1：空 body task → 用于测量"最小帧"
// =============================================================================
inline task<int> empty_body() { co_return 0; }

// =============================================================================
// 实验 2：含 5 个 int 局部变量 → 对比帧增量
// =============================================================================
inline task<int> five_ints() {
    int a = 1, b = 2, c = 3, d = 4, e = 5;
    co_return a + b + c + d + e;
}

// =============================================================================
// 实验 3：含 co_await → 观察 HALO 是否仍能触发
// =============================================================================
struct ready_int {
    int v;
    bool await_ready() noexcept { return true; }
    void await_suspend(std::coroutine_handle<>) noexcept {}
    int  await_resume() noexcept { return v; }
};

inline task<int> with_await() {
    int x = co_await ready_int{10};
    int y = co_await ready_int{32};
    co_return x + y;
}

// =============================================================================
// 实验 4：跨 DLL 实验占位
//
// 在真实工程中，把 make_task() 编进一个 DLL，把 main() 编进 EXE；
// 观察当两端编译器/STL/Optimization Level 不一致时是否崩溃。
// 本骨架仅给出函数声明，实际跨模块构建脚本见 CMakeLists 注释。
//
// ⚠️ ABI 要点：协程**不能**有 C 链接（extern "C"）。协程会生成编译器内部的
//    ramp/resume/destroy 函数与 promise，依赖 C++ name mangling 与 ABI；
//    给它套 extern "C" 在标准里是 ill-formed（MSVC 报 warning C4190）。
//    这恰恰说明：协程类型是强 C++ ABI 绑定的，跨 DLL 必须两端同编译器/同 STL/同优化级别。
// __declspec(dllexport) 在生产环境中应该加，但本骨架只演示接口形状。
// =============================================================================
inline task<int> make_task_for_dll_demo() { co_return 42; }

// =============================================================================
// 报告辅助：打印关键尺寸
// 注意：sizeof(task<int>) 只反映 task 包装（一个 handle 指针），
// 协程帧的真实大小须在编译诊断输出中查找。
// =============================================================================
inline void print_size_report() {
    std::cout << "----- size report -----\n";
    std::cout << "  sizeof(task<int>)               = "
              << sizeof(task<int>) << " bytes\n";
    std::cout << "  sizeof(std::coroutine_handle<>) = "
              << sizeof(std::coroutine_handle<>) << " bytes\n";
    std::cout << "  alignof(task<int>::promise_type)= "
              << alignof(task<int>::promise_type) << " bytes\n";
    std::cout << "(协程帧真实大小须从编译器诊断 flag 输出读取)\n";
}

// =============================================================================
// 数据采集表骨架 —— 用于在不同编译器下手工填写
//
// | 指标                               | MSVC | Clang | GCC |
// | ---------------------------------- | ---- | ----- | --- |
// | 协程帧大小（empty_body）           |   ?  |   ?   |  ?  |
// | 协程帧大小（five_ints）            |   ?  |   ?   |  ?  |
// | HALO 触发（empty_body, sync）      |   ?  |   ?   |  ?  |
// | HALO 触发（with_await）            |   ?  |   ?   |  ?  |
// | 调试信息中 frame 变量可读性         |   ?  |   ?   |  ?  |
// | coroutine_handle<>::sizeof          |   ?  |   ?   |  ?  |
//
// 备注：
//   - MSVC 帧大小：用 `/d1reportSingleClassLayout<promise_type>` 读 promise 偏移；
//   - Clang HALO：编译时若看见 "remark: coroutine frame elided" 即触发；
//   - GCC HALO ：用 `-fdump-ipa-coro` dump 后查 "elided"。
// =============================================================================

int main() {
    std::cout << "===== Exercise J-2: Cross-Compiler ABI =====\n";

    // 实验 1：空 body
    {
        auto t = empty_body();
        std::cout << "[empty_body] result = " << t.blocking_run() << "\n";
    }

    // 实验 2：五个 int 局部
    {
        auto t = five_ints();
        std::cout << "[five_ints]  result = " << t.blocking_run() << "\n";
    }

    // 实验 3：含 co_await
    {
        auto t = with_await();
        std::cout << "[with_await] result = " << t.blocking_run() << "\n";
    }

    // 实验 4：跨 DLL 占位
    {
        auto t = make_task_for_dll_demo();
        std::cout << "[dll_demo]   result = " << t.blocking_run() << "\n";
    }

    print_size_report();

    // ----------------------------------------------------------------------
    // 进阶实验（仅在多编译器环境中手工执行）：
    //
    //   1. 把 make_task_for_dll_demo() 拆到一个 DLL，main 在 EXE 中调用，
    //      DLL 用 Debug、EXE 用 Release，观察是否仍能 sync_wait 成功；
    //
    //   2. 把 coroutine_handle<promise_type> 通过函数参数传出 DLL，
    //      在 EXE 中调用 .destroy() —— 观察堆损坏 / 崩溃签名；
    //
    //   3. 阅读 MSVC `/await:strict` 和 `/await` 在 final_suspend 异常路径
    //      上的行为差异。
    //
    // 把每次实验的崩溃签名 / 帧大小 / HALO 触发结果写入 README 的对比表。
    // ----------------------------------------------------------------------

    std::cout << "===== Done =====\n";
    return 0;
}
