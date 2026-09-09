// =====================================================================
// 第一阶段结课：异步小爬虫
// 文档参考：C09_Coroutines/05-第一阶段结课-异步小爬虫.md
// 官方参考：
//   - cppreference <stop_token>, <generator>, <coroutine>
//   - P3149R4 async scope；P3296R1 let_async_scope
//   - Lewis Baker, "Structured Concurrency" CppCon 2019
//
// 项目目标：
//   并发抓取 N 个"URL"（字符串模拟）-> 用 std::generator 按行解析
//   -> 用 when_all 汇合 -> 用 stop_token 实现整体超时
//   -> 用 scope 收束所有 fire-and-forget 协程。
//
// 设计约束（来自结课文档）：
//   - 三个处理阶段：fetch / parse / aggregate
//   - 至少 1 次 stop_token 检查
//   - 至少 1 次 when_all 或 when_any
//   - 必须用 scope 管理并发 fetch；禁止 detach / 无 owner 的裸 thread / 全局可变状态
//
// 协程调用图（必填——请补充到下方注释中）：
//   main -> aggregate -> when_all{ fetch_one * N }
//                   \-> for each result: parse_lines (generator)
//   timeout_watchdog -> sleep(N) -> stop_source.request_stop()
//   task_scope: 持有并发 fetch 任务，join 后消费汇总结果。
// =====================================================================
#include "coroutine_study/lazy_task.hpp"
#include "coroutine_study/runtime.hpp"
#include "url_table.hpp"
#include "parser.hpp"

#include <chrono>
#include <coroutine>
#include <generator>
#include <iostream>
#include <mutex>
#include <optional>
#include <print>
#include <stop_token>
#include <string>
#include <thread>
#include <utility>
#include <vector>

using namespace std::chrono_literals;
using coroutine_study::lazy_task;

// ---------------------------------------------------------------------
// worker_group：timer awaiter 的后台线程 owner。
// ---------------------------------------------------------------------
struct worker_group {
    ~worker_group() { join(); }

    template <class Fn>
    void submit(Fn&& fn) {
        std::lock_guard lock(mutex_);
        workers_.emplace_back(std::forward<Fn>(fn));
    }

    void join() {
        for (;;) {
            std::vector<std::jthread> local;
            {
                std::lock_guard lock(mutex_);
                if (workers_.empty()) break;
                local.swap(workers_);
            }
            for (auto& worker : local) {
                if (worker.joinable()) worker.join();
            }
        }
    }

private:
    std::mutex mutex_;
    std::vector<std::jthread> workers_;
};

// ---------------------------------------------------------------------
// timer_awaiter：由 worker_group 拥有等待线程，并观察 stop_token。
// ---------------------------------------------------------------------
struct timer_awaiter {
    worker_group& workers;
    std::chrono::milliseconds dur;
    std::stop_token st;

    bool await_ready() const noexcept { return dur <= 0ms || st.stop_requested(); }
    void await_suspend(std::coroutine_handle<> h) const {
        workers.submit([h, dur = dur, st = st] {
            auto end = std::chrono::steady_clock::now() + dur;
            while (!st.stop_requested() && std::chrono::steady_clock::now() < end) {
                std::this_thread::sleep_for(2ms);
            }
            h.resume();
        });
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
fetch_one(worker_group& workers, capstone1::UrlRecord rec, std::stop_token st) {
    auto t0 = std::chrono::steady_clock::now();

    // TODO [必做 1]：在 co_await 前检查 st.stop_requested()
    //   -> co_return FetchResult{ rec.url, "", Stopped, 0ms };

    // TODO [必做 2]：co_await timer_awaiter{workers, rec.latency, st} 模拟网络延迟。
    co_await timer_awaiter{workers, rec.latency, st};

    // TODO [必做 3]：co_await 之后再次检查 stop_token
    //   被取消则返回 Stopped 状态。

    auto dt = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - t0);
    co_return FetchResult{rec.url, rec.body, FetchStatus::Ok, dt};
}

// ---------------------------------------------------------------------
// 简化版 when_all：等齐 N 个 fetch_one。
// 学习版直接顺序 sync_wait，请改写为并行版本。
// ---------------------------------------------------------------------
lazy_task<std::vector<FetchResult>>
when_all_fetch(worker_group& workers, std::vector<capstone1::UrlRecord> recs, std::stop_token st) {
    std::vector<FetchResult> out;
    out.reserve(recs.size());
    // TODO [必做 4]：先把 out 调整到 recs.size()，再创建 coroutine_study::task_scope。
    //   参照 solution.cpp 的 fetch_into，为各索引创建一个写回结果的 lazy_task<void>，
    //   通过 scope.spawn(...) 接管任务；scope.join() 后再 co_return out。
    //   每个任务写不同索引，任务运行期间保持 out 的容量与元素位置稳定。
    for (auto& r : recs) {
        auto task = fetch_one(workers, r, st);
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
    int  total_score = 0;
    std::chrono::milliseconds elapsed{0};
    std::vector<FetchResult>  per_url;
};

// ---------------------------------------------------------------------
// aggregate：汇合 + 解析 + 统计
// ---------------------------------------------------------------------
lazy_task<Report>
aggregate(worker_group& workers, std::vector<capstone1::UrlRecord> recs, std::stop_token st) {
    auto t0 = std::chrono::steady_clock::now();

    // TODO [必做 5]：用 when_all_fetch 并行抓取，逐 URL 用 parse_lines
    //   计数行数；按状态分流到 ok/stopped/err。
    auto fetches = co_await when_all_fetch(workers, recs, st);

    Report rep;
    rep.total = static_cast<int>(fetches.size());
    rep.per_url = fetches;
    for (auto& f : fetches) {
        switch (f.status) {
            case FetchStatus::Ok:
                ++rep.ok_count;
                for (auto&& r : capstone1::parse_lines(f.body)) {
                    if (r.ok) {
                        ++rep.total_lines;
                        // TODO [必做 6]：parser.hpp 完成 score 解析后，把有效分数累加到 total_score。
                        rep.total_score += r.score;
                    }
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
    std::println("总分: {}", r.total_score);
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
    worker_group workers;

    // TODO [必做 7]：观察 watchdog 线程在 200ms 时请求停止。
    //   完成并发 when_all_fetch 后，对照 reference 的两条成功、一条停止结果。
    std::jthread watchdog([&src](std::stop_token jt) {
        std::this_thread::sleep_for(200ms);
        if (!jt.stop_requested()) src.request_stop();
    });

    // ------------------ 主流程 ------------------
    auto task = aggregate(workers, recs, token);
    Report rep = coroutine_study::sync_wait(std::move(task));
    workers.join();
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
