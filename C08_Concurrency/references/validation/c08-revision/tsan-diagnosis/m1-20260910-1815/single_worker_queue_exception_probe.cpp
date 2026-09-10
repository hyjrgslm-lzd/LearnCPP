#include <chrono>
#include <condition_variable>
#include <deque>
#include <exception>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string_view>
#include <thread>
#include <vector>

class single_worker_queue {
    using task = std::function<void()>;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::deque<task> tasks_;
    bool closed_ = false;
    std::thread worker_;

    void run() {
        for (;;) {
            task work;
            {
                std::unique_lock lock(mutex_);
                cv_.wait(lock, [&] { return closed_ || !tasks_.empty(); });
                if (tasks_.empty()) return;
                work = std::move(tasks_.front());
                tasks_.pop_front();
            }
            work();
        }
    }

public:
    single_worker_queue() : worker_([this] { run(); }) {}
    ~single_worker_queue() { join(); }

    template<class F>
    auto submit(F&& f) {
        using result = std::invoke_result_t<std::decay_t<F>&>;
        auto packaged = std::make_shared<std::packaged_task<result()>>(std::forward<F>(f));
        auto future = packaged->get_future();
        task wrapped = [packaged] { (*packaged)(); };
        {
            std::lock_guard lock(mutex_);
            tasks_.push_back(std::move(wrapped));
        }
        cv_.notify_one();
        return future;
    }

    void join() {
        {
            std::lock_guard lock(mutex_);
            closed_ = true;
        }
        cv_.notify_one();
        if (worker_.joinable()) worker_.join();
    }
};

int exception_round(bool join_before_what) {
    single_worker_queue queue;
    auto error = queue.submit([]() -> int { throw std::runtime_error("task-error"); });
    bool caught = false;
    if (join_before_what) {
        error.wait();
        queue.join();
    }
    try {
        (void)error.get();
    } catch (const std::runtime_error& e) {
        caught = std::string_view(e.what()) == "task-error";
        std::this_thread::sleep_for(std::chrono::microseconds(50));
        caught = caught && std::string_view(e.what()) == "task-error";
    }
    queue.join();
    return caught ? 0 : 1;
}

int value_round() {
    single_worker_queue queue;
    auto answer = queue.submit([] { return 42; });
    const int value = answer.get();
    queue.join();
    return value == 42 ? 0 : 1;
}

int main(int argc, char** argv) {
    const std::string_view mode = argc > 1 ? argv[1] : "exception";
    const int rounds = argc > 2 ? std::stoi(argv[2]) : 200;
    for (int i = 0; i < rounds; ++i) {
        int rc = 1;
        if (mode == "exception") rc = exception_round(false);
        else if (mode == "join-before-what") rc = exception_round(true);
        else if (mode == "value") rc = value_round();
        else return 2;
        if (rc != 0) return rc;
    }
    std::cout << "ok " << mode << " rounds=" << rounds << '\n';
}
