// F-1 观察协程帧布局
// 文档参考：08-模块F-协程帧与allocator.md 「练习 F-1」
// 官方参考：
//   - GCC `-fdump-tree-all` 文档
//   - MSVC `/d1reportSingleClassLayout` 调试 flag
//   - Clang `-Xclang -ast-dump`
//   - Gor Nishanov "C++ Coroutines: Under the covers" CppCon 2016
//
// 目标：写一个含丰富局部变量与多个 co_await 点的协程，让编译器生成的
//      coroutine frame 结构体可观测。完成必做任务后请去查 dump 文件，
//      在帧中标注：promise / 参数副本 / resume_index / 局部变量 spill。

#include <coroutine>
#include <cstdio>
#include <string>
#include <vector>
#include <memory>
#include <utility>

// ========== 被观察的 task 类型 ==========
struct observer_task {
    struct promise_type {
        // 标记字段，便于在 dump 文件里搜索
        int promise_marker_ = 0xCAFE;

        observer_task get_return_object() noexcept {
            return observer_task{
                std::coroutine_handle<promise_type>::from_promise(*this)
            };
        }
        std::suspend_never  initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend()   noexcept { return {}; }
        void return_void() noexcept {}
        void unhandled_exception() noexcept {}
    };
    std::coroutine_handle<promise_type> h_{};
    observer_task() = default;
    explicit observer_task(std::coroutine_handle<promise_type> h) noexcept : h_(h) {}
    observer_task(observer_task&& o) noexcept : h_(std::exchange(o.h_, {})) {}
    ~observer_task() { if (h_) h_.destroy(); }   // 关键：避免帧泄漏
};

// ========== 多 co_await 点 + 多类型局部变量的协程 ==========
// TODO [必做]：用 GCC -fdump-tree-all 编译该文件，
//   找到生成的 .c.022t.coro（或同类）dump 文件，搜索 _Coro_frame / __frame，
//   把 promise / param 副本 / resume_index / 局部变量 spill 的相对位置画下来。
observer_task observed(int param_a, double param_b, std::string param_c)
{
    int    local_x = param_a * 2;             // 平凡类型局部变量
    double local_y = local_x + param_b;       // 平凡类型局部变量
    co_await std::suspend_always{};           // 挂起点 1
    std::string local_str = param_c;          // 非平凡类型，需在帧析构里调析构
    co_await std::suspend_always{};           // 挂起点 2
    std::printf("[observed] %d %f %s\n",
                local_x, local_y, local_str.c_str());
    co_return;
}

// ========== 进阶：包含更复杂局部变量的协程 ==========
// TODO [进阶]：观察 vector / unique_ptr 在帧内的位置和大小。
observer_task observed_complex()
{
    std::vector<int>           buf{1, 2, 3, 4, 5};
    std::unique_ptr<double>    p = std::make_unique<double>(3.14);
    co_await std::suspend_always{};
    buf.push_back(static_cast<int>(*p));
    co_await std::suspend_always{};
    std::printf("[observed_complex] vec.size=%zu *p=%f\n", buf.size(), *p);
    co_return;
}

// ========== 进阶：单 co_await 点的极简协程，对比帧大小 ==========
observer_task observed_minimal()
{
    int x = 1;
    co_await std::suspend_always{};
    std::printf("[observed_minimal] x=%d\n", x);
    co_return;
}

// ========== 驱动器：手动 resume 三个协程到底 ==========
// 注：observer_task::initial_suspend 是 suspend_never，所以协程会立即开始执行
//    直到第一个 co_await 点。我们只需保存其 handle 并继续 resume 即可。
//    这里为简单起见不保存 handle——本题目的是观察 dump，不是观察执行。
int main()
{
    std::printf("===== F-1: 观察协程帧布局 =====\n");

    // TODO [必做]：用编译器 flag 生成 dump，再回到这里读 dump。
    // 注意：本 main 不会真的把协程跑到底——initial_suspend = suspend_never，
    //      但每个协程在第一次 co_await suspend_always{} 后会暂停。
    //      帧已经被构造，够 dump 工具观察了。
    auto t1 = observed(7, 1.5, std::string{"hello"});
    auto t2 = observed_complex();
    auto t3 = observed_minimal();
    (void)t1; (void)t2; (void)t3;

    std::printf("\n[提示] 帧 dump 操作（任选其一）：\n");
    std::printf("  GCC  : g++ -std=c++23 -fdump-tree-all main.cpp\n");
    std::printf("         查 .c.022t.coro 文件 → 搜索 _Coro_frame\n");
    std::printf("  MSVC : cl /std:c++latest /d1reportSingleClassLayout<name> main.cpp\n");
    std::printf("  Clang: clang++ -std=c++23 -Xclang -ast-dump -fsyntax-only main.cpp\n");

    // TODO [复盘]：在自己的笔记里画 ASCII 帧布局图，
    //   并回答：为什么参数 std::string param_c 的副本必须在帧上而非栈上？
    //          为什么 resume_index 必须是帧内字段，而不能由 IP 推算？
    return 0;
}
