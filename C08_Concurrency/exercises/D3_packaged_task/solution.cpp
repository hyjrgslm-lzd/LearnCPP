#include "concurrency_study/exercise_check.hpp"
#include <chrono>
#include <condition_variable>
#include <exception>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

using namespace std::chrono_literals;
struct job_failure : std::runtime_error { using std::runtime_error::runtime_error; };
struct submission_failure : std::runtime_error { using std::runtime_error::runtime_error; };

void part1_direct_and_thread() {
    const auto caller = std::this_thread::get_id();
    std::packaged_task<std::thread::id()> direct([] { return std::this_thread::get_id(); });
    auto local = direct.get_future();
    cs::check(local.wait_for(0s) == std::future_status::timeout, "packaging does not execute");
    direct();
    cs::check(local.get() == caller, "operator() runs on calling thread");

    std::packaged_task<std::thread::id()> task([] { return std::this_thread::get_id(); });
    auto result = task.get_future();
    std::exception_ptr error;
    std::jthread worker([t = std::move(task), &error]() mutable {
        try { t(); }
        catch (...) { error = std::current_exception(); } // Invocation protocol failures.
    });
    worker.join();
    if (error) std::rethrow_exception(error);
    cs::check(!task.valid() && result.get() != caller, "task moved to explicit worker");
    std::cout << "Part 1: same task abstraction, caller or worker selected by invocation\n";
}

void part2_and_3_batch_queue() {
    // ponytail: closed batch, one worker; use the channel chapter's close/wait
    // protocol if producers must overlap consumers. This is not a thread pool.
    std::queue<std::packaged_task<void()>> queue;
    std::vector<std::future<int>> results;
    std::vector<std::future<void>> envelopes;
    for (int id = 0; id < 6; ++id) {
        std::packaged_task<int()> task([id] {
            if (id == 3) throw job_failure("job 3 failed");
            return id * 100;
        });
        results.push_back(task.get_future());
        auto invoke_task = [t = std::move(task)]() mutable { t(); };
        static_assert(!std::is_copy_constructible_v<decltype(invoke_task)>);
        std::packaged_task<void()> envelope(std::move(invoke_task));
        envelopes.push_back(envelope.get_future());
        queue.push(std::move(envelope));
    }
    auto pointer = std::make_unique<int>(7);
    std::packaged_task<std::string()> text_task([p = std::move(pointer)] {
        return std::string("item-") + std::to_string(*p);
    });
    auto text_result = text_task.get_future();
    std::packaged_task<void()> text_envelope([t = std::move(text_task)]() mutable { t(); });
    envelopes.push_back(text_envelope.get_future());
    queue.push(std::move(text_envelope));
    cs::check(!pointer && queue.size() == 7, "heterogeneous move-only tasks queued");
    for (auto& result : results)
        cs::check(result.wait_for(0s) == std::future_status::timeout, "submitted is not executed");

    std::exception_ptr infrastructure_error;
    int completed = 0;
    std::jthread worker([&] {
        try {
            while (!queue.empty()) {
                auto task = std::move(queue.front());
                queue.pop();
                task();
                ++completed;
            }
        } catch (...) { infrastructure_error = std::current_exception(); }
    });
    worker.join();
    if (infrastructure_error) std::rethrow_exception(infrastructure_error);
    cs::check(queue.empty() && completed == 7, "closed batch drained exactly once");
    // Outer packaged_task also has a channel: inspect it to avoid hiding errors.
    for (auto& envelope : envelopes) envelope.get();
    int failures = 0;
    for (int id = 0; id < 6; ++id) {
        bool failed = false;
        int value = -1;
        try { value = results[id].get(); }
        catch (const job_failure& e) {
            failed = true;
            cs::check(std::string(e.what()) == "job 3 failed", "original exception message");
            ++failures;
        }
        cs::check(failed == (id == 3), "only selected job fails");
        if (!failed) cs::check(value == id * 100, "individual result identity");
    }
    cs::check(failures == 1 && text_result.get() == "item-7", "heterogeneous result and failure");
    std::cout << "Parts 2/3: 7 queued, 7 executed; int/string results, job 3 exception\n";
}

void part4_one_shot_and_abandon() {
    std::packaged_task<int()> once([] { return 42; });
    auto result = once.get_future();
    once();
    bool rejected = false;
    try { once(); }
    catch (const std::future_error& e) { rejected = e.code() == std::future_errc::promise_already_satisfied; }
    cs::check(rejected && result.get() == 42, "one invocation per shared state");
    std::future<int> discarded;
    {
        std::packaged_task<int()> never_run([] { return 0; });
        discarded = never_run.get_future();
    }
    bool broken = false;
    try { (void)discarded.get(); }
    catch (const std::future_error& e) { broken = e.code() == std::future_errc::broken_promise; }
    cs::check(broken, "discarded task abandons provider state");
    std::cout << "Part 4: repeated invocation rejected; discarded task -> broken_promise\n";
}

void part5_online_queue(int count, bool fail_before_release = false) {
    // Scene change from the closed batch: producer and consumer may overlap.
    std::mutex mutex;
    std::condition_variable changed;
    std::queue<std::packaged_task<int()>> queue;
    bool closed = false;
    auto close = [&] {
        { std::lock_guard lock(mutex); closed = true; }
        changed.notify_all();
    };
    auto enqueue = [&](std::packaged_task<int()> task) {
        {
            std::lock_guard lock(mutex);
            if (closed) return false;
            queue.push(std::move(task));
        }
        changed.notify_one();
        return true;
    };
    std::vector<std::future<int>> results;
    std::vector<int> execution_order;
    execution_order.reserve(count);
    std::exception_ptr worker_error;
    std::exception_ptr submission_error;
    int completed = 0;
    std::jthread worker;
    {
        // Release provider must be destroyed BEFORE joining worker, including
        // check/allocation failures. The worker owns a copy of the gate handle.
        std::promise<void> release;
        auto gate = release.get_future().share();
        std::promise<void> entered;
        auto started = entered.get_future();
        worker = std::jthread([&, gate](std::promise<void> signal) {
            try {
                signal.set_value();
                gate.wait(); // Readiness only: abandonment also releases the gate.
                for (;;) {
                    std::packaged_task<int()> task;
                    {
                        std::unique_lock lock(mutex);
                        changed.wait(lock, [&] { return closed || !queue.empty(); });
                        if (queue.empty()) break; // Predicate implies closed here.
                        task = std::move(queue.front());
                        queue.pop();
                    }
                    task(); // Invoke outside the queue lock.
                    ++completed;
                }
            } catch (...) { worker_error = std::current_exception(); }
        }, std::move(entered));
        try {
            started.get(); // Worker has reached the gate protocol, before any pop.
            for (int id = 0; id < count; ++id) {
                std::packaged_task<int()> task([id, &execution_order] {
                    execution_order.push_back(id); // Actual invocation, before possible failure.
                    if (id == 3) throw job_failure("online job 3 failed");
                    return id * 10;
                });
                results.push_back(task.get_future());
                cs::check(enqueue(std::move(task)), "open queue accepts submission");
            }
            close();
            {
                std::lock_guard lock(mutex);
                cs::check(closed && queue.size() == static_cast<std::size_t>(count),
                          "close observes the entire pending batch before gate release");
            }
            for (auto& result : results)
                cs::check(result.wait_for(0s) == std::future_status::timeout, "closed batch still pending");
            std::packaged_task<int()> rejected([] { return -1; });
            auto rejected_result = rejected.get_future();
            cs::check(!enqueue(std::move(rejected)), "closed queue rejects");
            cs::check(rejected_result.wait_for(0s) == std::future_status::ready, "rejected task abandoned");
            bool broken = false;
            try { (void)rejected_result.get(); }
            catch (const std::future_error& e) { broken = e.code() == std::future_errc::broken_promise; }
            cs::check(broken, "rejected submission reports broken_promise");
            if (fail_before_release) throw submission_failure("injected before release");
            release.set_value();
        } catch (...) {
            submission_error = std::current_exception();
            close(); // Also release a consumer waiting on the queue predicate.
        }
    } // Unsatisfied release publishes broken_promise BEFORE worker.join().
    worker.join();
    if (worker_error) std::rethrow_exception(worker_error);
    bool injected = false;
    try { if (submission_error) std::rethrow_exception(submission_error); }
    catch (const submission_failure& e) {
        injected = true;
        cs::check(std::string(e.what()) == "injected before release", "original submission failure");
    }
    cs::check(injected == fail_before_release, "injected failure reached caller after cleanup");
    cs::check(queue.empty() && completed == count, "close drains all accepted tasks");
    cs::check(execution_order.size() == static_cast<std::size_t>(count), "every task recorded execution");
    for (int id = 0; id < count; ++id) {
        cs::check(execution_order[id] == id, "actual invocation order is FIFO");
        int value = -1;
        bool failed = false;
        try { value = results[id].get(); }
        catch (const job_failure& e) {
            failed = true;
            cs::check(std::string(e.what()) == "online job 3 failed", "online exception identity");
        }
        cs::check(failed == (id == 3), "online failure belongs to its task");
        if (!failed) cs::check(value == id * 10, "online individual result");
    }
    std::cout << "Part 5: pending at close=" << count << ", executed IDs:";
    for (int id : execution_order) std::cout << ' ' << id;
    std::cout << "; rejected=broken_promise; release=" << (injected ? "abandoned" : "value") << '\n';
}

int main() {
    part1_direct_and_thread();
    part2_and_3_batch_queue();
    part4_one_shot_and_abandon();
    part5_online_queue(0);
    part5_online_queue(4);
    part5_online_queue(4, true);
    std::cout << "D3_reference OK\n";
}
