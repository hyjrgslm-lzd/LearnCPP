#include "coroutine_study/exercise_check.hpp"
#include "coroutine_study/lazy_task.hpp"
#include "coroutine_study/runtime.hpp"
#include "url_table.hpp"

#include <charconv>
#include <chrono>
#include <coroutine>
#include <generator>
#include <iostream>
#include <mutex>
#include <stop_token>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

namespace {

struct worker_group {
    ~worker_group() { join(); }
    template <class Fn>
    void submit(Fn&& fn) {
        std::lock_guard lock(mutex);
        workers.emplace_back(std::forward<Fn>(fn));
    }
    void join() {
        for (;;) {
            std::vector<std::jthread> local;
            {
                std::lock_guard lock(mutex);
                if (workers.empty()) break;
                local.swap(workers);
            }
            for (auto& worker : local) if (worker.joinable()) worker.join();
        }
    }
    std::mutex mutex;
    std::vector<std::jthread> workers;
};

struct csv_record {
    std::string name;
    int score = 0;
    bool ok = true;
};

std::generator<csv_record> parse_csv(std::string_view body) {
    std::size_t pos = 0;
    bool header = true;
    while (pos <= body.size()) {
        std::size_t nl = body.find('\n', pos);
        std::string_view line = nl == std::string_view::npos ? body.substr(pos) : body.substr(pos, nl - pos);
        pos = nl == std::string_view::npos ? body.size() + 1 : nl + 1;
        if (line.empty()) continue;
        if (header) {
            header = false;
            if (line != "name,score") co_yield csv_record{"", 0, false};
            continue;
        }
        std::size_t comma = line.find(',');
        if (comma == std::string_view::npos || comma == 0 || comma + 1 == line.size()) {
            co_yield csv_record{std::string{line}, 0, false};
            continue;
        }
        int score = 0;
        auto score_text = line.substr(comma + 1);
        auto [ptr, ec] = std::from_chars(score_text.data(), score_text.data() + score_text.size(), score);
        if (ec != std::errc{} || ptr != score_text.data() + score_text.size()) {
            co_yield csv_record{std::string{line.substr(0, comma)}, 0, false};
            continue;
        }
        co_yield csv_record{std::string{line.substr(0, comma)}, score, true};
    }
}

struct timer_awaiter {
    worker_group& workers;
    std::chrono::milliseconds d;
    std::stop_token st;
    bool await_ready() const noexcept { return d <= 0ms || st.stop_requested(); }
    void await_suspend(std::coroutine_handle<> h) const {
        workers.submit([h, d = d, st = st] {
            auto end = std::chrono::steady_clock::now() + d;
            while (!st.stop_requested() && std::chrono::steady_clock::now() < end) {
                std::this_thread::sleep_for(2ms);
            }
            h.resume();
        });
    }
    void await_resume() const noexcept {}
};

enum class fetch_status { ok, stopped, error };

struct fetch_result {
    std::string url;
    std::string body;
    fetch_status status = fetch_status::ok;
    std::chrono::milliseconds elapsed{0};
};

struct report {
    int total = 0;
    int ok_count = 0;
    int stopped = 0;
    int err = 0;
    int total_lines = 0;
    int total_score = 0;
    std::chrono::milliseconds elapsed{0};
    std::vector<fetch_result> per_url;
};

coroutine_study::lazy_task<fetch_result> fetch_one(worker_group& workers, capstone1::UrlRecord rec, std::stop_token st) {
    auto start = std::chrono::steady_clock::now();
    if (st.stop_requested()) co_return fetch_result{rec.url, "", fetch_status::stopped, 0ms};
    co_await timer_awaiter{workers, rec.latency, st};
    auto dt = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);
    if (st.stop_requested()) co_return fetch_result{rec.url, "", fetch_status::stopped, dt};
    co_return fetch_result{rec.url, rec.body, fetch_status::ok, dt};
}

coroutine_study::lazy_task<void> fetch_into(
    worker_group& workers,
    capstone1::UrlRecord rec,
    std::stop_token st,
    std::vector<fetch_result>& out,
    std::size_t index
) {
    try {
        out[index] = co_await fetch_one(workers, std::move(rec), st);
    } catch (...) {
        out[index] = fetch_result{rec.url, "", fetch_status::error, 0ms};
    }
}

coroutine_study::lazy_task<std::vector<fetch_result>> when_all_fetch(
    worker_group& workers,
    std::vector<capstone1::UrlRecord> recs,
    std::stop_token st
) {
    std::vector<fetch_result> out(recs.size());
    coroutine_study::task_scope scope;
    for (std::size_t i = 0; i < recs.size(); ++i) {
        scope.spawn(fetch_into(workers, recs[i], st, out, i));
    }
    scope.join();
    co_return out;
}

coroutine_study::lazy_task<report> aggregate(worker_group& workers, std::vector<capstone1::UrlRecord> recs, std::stop_token st) {
    auto start = std::chrono::steady_clock::now();
    auto fetches = co_await when_all_fetch(workers, std::move(recs), st);
    report rep;
    rep.total = static_cast<int>(fetches.size());
    rep.per_url = fetches;
    for (const auto& f : fetches) {
        switch (f.status) {
            case fetch_status::ok:
                ++rep.ok_count;
                for (auto line : parse_csv(f.body)) {
                    if (line.ok) {
                        ++rep.total_lines;
                        rep.total_score += line.score;
                    } else {
                        ++rep.err;
                    }
                }
                break;
            case fetch_status::stopped:
                ++rep.stopped;
                break;
            case fetch_status::error:
                ++rep.err;
                break;
        }
    }
    rep.elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);
    co_return rep;
}

} // namespace

int main() {
    using coroutine_study::check;

    std::vector<csv_record> parsed;
    for (auto row : parse_csv("name,score\nAlice,85\nbad\nEve,nope\nBob,92\n")) parsed.push_back(row);
    check(parsed.size() == 4, "parser yields all non-empty data rows");
    check(parsed[0].ok && parsed[0].name == "Alice" && parsed[0].score == 85, "parser reads name and score");
    check(!parsed[1].ok && !parsed[2].ok, "parser marks malformed rows");
    check(parsed[3].ok && parsed[3].name == "Bob" && parsed[3].score == 92, "parser resumes after malformed rows");

    worker_group workers;
    std::stop_source src;
    std::jthread watchdog([&] {
        std::this_thread::sleep_for(200ms);
        src.request_stop();
    });
    auto task = aggregate(workers, capstone1::url_table(), src.get_token());
    auto rep = coroutine_study::sync_wait(std::move(task));
    workers.join();

    check(rep.total == static_cast<int>(capstone1::url_table().size()), "all urls accounted for");
    check(rep.ok_count == 2, "two fast urls complete before timeout");
    check(rep.stopped == 1, "one slow url is stopped");
    check(rep.err == 0, "sample table has no parse errors");
    check(rep.total_lines == 4, "parser counts successful rows precisely");
    check(rep.total_score == 343, "parser preserves numeric scores");
    check(rep.elapsed < 350ms, "fetches run concurrently under timeout");
    std::cout << "Capstone1_reference OK\n";
}
