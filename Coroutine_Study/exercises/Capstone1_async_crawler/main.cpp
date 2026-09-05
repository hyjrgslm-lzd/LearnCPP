// =====================================================================
// 第一阶段结课：异步小爬虫
// 文档参考：Coroutine_Study/05-第一阶段结课-异步小爬虫.md
// 官方参考：
//   - cppreference <stop_token>, <generator>, <coroutine>
//   - P3149R4 async scope；P3296R1 let_async_scope
//   - Lewis Baker, "Structured Concurrency" CppCon 2019
//
// 项目目标：
//   并发抓取 N 个"URL"（字符串模拟）-> 用 std::generator 按行解析
//   -> 用 when_all 汇合 -> 用 stop_token 实现整体超时
//   -> 用 async_scope 收束所有 fire-and-forget 协程。
//
// 设计约束（来自结课文档）：
//   - 至少 3 类异步阶段：fetch / parse / aggregate
//   - 至少 1 次 stop_token 检查
//   - 至少 1 次 when_all 或 when_any
//   - 必须用 async_scope 管理并发 fetch；禁止 detach / 裸 thread / 全局可变状态
//
// 协程调用图（必填——请补充到下方注释中）：
//   main -> aggregate -> when_all{ fetch_one * N }
//                   \-> for each result: parse_lines (generator)
//   timeout_watchdog -> sleep(N) -> stop_source.request_stop()
//   async_scope: 持有 fetch_one 的所有协程，析构时等齐
// =====================================================================
#include "coroutine_study/lazy_task.hpp"
#include "url_table.hpp"
#include "parser.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <coroutine>
#include <generator>
#include <iostream>
#include <mutex>
#include <optional>
#include <print>
#include <stop_token>
#include <string>
#include <thread>
#include <vector>

using namespace std::chrono_literals;
using coroutine_study::lazy_task;

// ---------------------------------------------------------------------
// async_sleep：可被 stop_token 唤醒的版本（提示性骨架）。
// ---------------------------------------------------------------------
struct async_sleep {
    std::chrono::milliseconds dur;
    bool await_ready() const noexcept { return dur <= 0ms; }
    void await_suspend(std::coroutine_handle<> h) const {
        std::this_thread::sleep_for(dur);
        h.resume();
    }
    void await_resume() const noexcept {}
};

// ---------------------------------------------------------------------
// fetch 结果结构
// ---------------------------------------------------------------------
enum class FetchStatus { Ok, Stopped, Error };

struct FetchResult {
    std::string  url;
    std::string  body;
    FetchStatus  status   = FetchStatus::Ok;
    std::chrono::milliseconds elapsed{0};
};

// ---------------------------------------------------------------------
// fetch_one：单个 URL 的拉取
// ---------------------------------------------------------------------
lazy_task<FetchResult>
fetch_one(capstone1::UrlRecord rec, std::stop_token st) {
    auto t0 = std::chrono::steady_clock::now();

    // TODO [必做 1]：在 co_await 前检查 st.stop_requested()
    //   -> co_return FetchResult{ rec.url, "", Stopped, 0ms };

    // TODO [必做 2]：co_await async_sleep{rec.latency} 模拟网络延迟。
    co_await async_sleep{rec.latency};

    // TODO [必做 3]：co_await 之后再次检查 stop_token
    //   被取消则返回 Stopped 状态。

    auto dt = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - t0);
    co_return FetchResult{rec.url, rec.body, FetchStatus::Ok, dt};
}

// ---------------------------------------------------------------------
// 极简 async_scope：与 C-3 对应。
// 命名说明：C-3 的 scope.spawn(task) 接收一个已构造的 task 并接管其所有权；
//   这里的 spawn_with(fn) 接收一个"工厂可调用对象"，在新线程里调用 fn() 启动工作，
//   便于在循环里按需多次启动。两者意图相同（fire-and-forget + 汇合等待），
//   只是入参形态不同。
// ---------------------------------------------------------------------
class async_scope {
public:
    template <typename Fn>
    void spawn_with(Fn&& fn) {
        in_flight_.fetch_add(1, std::memory_order_relaxed);
        workers_.emplace_back([this, fn = std::forward<Fn>(fn)]() mutable {
            try { fn(); } catch (...) {}
            if (in_flight_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
                std::lock_guard<std::mutex> lk(mu_);
                cv_.notify_all();
            }
        });
    }
    ~async_scope() {
        std::unique_lock<std::mutex> lk(mu_);
        cv_.wait(lk, [this] { return in_flight_.load(std::memory_order_acquire) == 0; });
    }
private:
    std::atomic<int> in_flight_{0};
    std::mutex mu_;
    std::condition_variable cv_;
    std::vector<std::jthread> workers_;
};

// ---------------------------------------------------------------------
// 简化版 when_all：等齐 N 个 fetch_one。
// 学习版直接顺序 sync_wait，请改写为并行版本。
// ---------------------------------------------------------------------
lazy_task<std::vector<FetchResult>>
when_all_fetch(std::vector<capstone1::UrlRecord> recs, std::stop_token st) {
    std::vector<FetchResult> out;
    out.reserve(recs.size());
    // TODO [必做 4]：把每个 fetch_one 通过 async_scope.spawn_with 并行启动，
    //   收集结果到 out（注意线程安全），最后 co_return out。
    for (auto& r : recs) {
        auto task = fetch_one(r, st);
        out.push_back(coroutine_study::sync_wait(std::move(task))); // 占位：串行。请并行化。
    }
    co_return out;
}

// ---------------------------------------------------------------------
// 报告结构
// ---------------------------------------------------------------------
struct Report {
    int  total       = 0;
    int  ok_count    = 0;
    int  stopped     = 0;
    int  err         = 0;
    int  total_lines = 0;
    std::chrono::milliseconds elapsed{0};
    std::vector<FetchResult>  per_url;
};

// ---------------------------------------------------------------------
// aggregate：汇合 + 解析 + 统计
// ---------------------------------------------------------------------
lazy_task<Report>
aggregate(std::vector<capstone1::UrlRecord> recs, std::stop_token st) {
    auto t0 = std::chrono::steady_clock::now();

    // TODO [必做 5]：用 when_all_fetch 并行抓取，逐 URL 用 parse_lines
    //   计数行数；按状态分流到 ok/stopped/err。
    auto fetches = co_await when_all_fetch(recs, st);

    Report rep;
    rep.total = static_cast<int>(fetches.size());
    rep.per_url = fetches;
    for (auto& f : fetches) {
        switch (f.status) {
            case FetchStatus::Ok:
                ++rep.ok_count;
                for (auto&& r : capstone1::parse_lines(f.body)) {
                    if (r.ok) ++rep.total_lines;
                }
                break;
            case FetchStatus::Stopped: ++rep.stopped; break;
            case FetchStatus::Error:   ++rep.err; break;
        }
    }
    rep.elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - t0);
    co_return rep;
}

// ---------------------------------------------------------------------
// 输出报告
// ---------------------------------------------------------------------
void print_report(const Report& r) {
    std::println("=== 异步小爬虫报告 ===");
    std::println("请求 URL 总数: {}", r.total);
    std::println("成功: {} | 超时: {} | 失败: {}", r.ok_count, r.stopped, r.err);
    std::println("总解析行数: {}", r.total_lines);
    std::println("整体耗时: {}ms\n", r.elapsed.count());
    for (auto& f : r.per_url) {
        const char* s = (f.status == FetchStatus::Ok) ? "成功"
                       : (f.status == FetchStatus::Stopped) ? "超时" : "失败";
        std::println("URL: {} | 状态: {} | 耗时: {}ms",
                     f.url, s, f.elapsed.count());
    }
}

int main() {
    std::println("===== Capstone 1：异步小爬虫 =====\n");

    auto recs = capstone1::url_table();

    // ------------------ 整体超时 ------------------
    std::stop_source src;
    auto token = src.get_token();

    // TODO [必做 7]：起一个超时 watchdog 协程：sleep N 后调用 src.request_stop()
    //   建议阈值 200ms（小于 remote 的 300ms，可以触发 1 次超时）。
    std::jthread watchdog([&src](std::stop_token jt) {
        std::this_thread::sleep_for(600ms);  // 占位串行版下让前两个 URL 跑完；并行实现后改回 200ms
        if (!jt.stop_requested()) src.request_stop();
    });

    // ------------------ 主流程 ------------------
    auto task = aggregate(recs, token);
    Report rep = coroutine_study::sync_wait(std::move(task));
    print_report(rep);

    // ------------------ 进阶任务 ------------------
    // TODO [进阶 1]：优雅降级——超时的 URL 不影响其它结果汇总。
    // TODO [进阶 2]：对超时的 URL 做一次自动重试（更短超时阈值）。
    // TODO [进阶 3]：用 when_any(when_all(fetches), timeout_after(N))
    //   替代 watchdog 协程。
    // TODO [进阶 4]：parse 阶段也异步化（边下载边解析）。
    // TODO [进阶 5]：写一份 1 页小结：接 libcurl/asio 时哪些结构可复用。

    std::println("\n===== Done =====");
    return 0;
}
