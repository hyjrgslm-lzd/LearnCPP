#include <cassert>
#include <chrono>
#include <exception>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <thread>
#include <vector>

int run_exception_once() {
    auto packaged = std::make_shared<std::packaged_task<int()>>(
        []() -> int { throw std::runtime_error("task-error"); });
    auto future = packaged->get_future();
    std::function<void()> work = [packaged] { (*packaged)(); };
    std::thread worker([work = std::move(work)] mutable {
        work();
    });
    bool caught = false;
    try {
        (void)future.get();
    } catch (const std::runtime_error& e) {
        caught = std::string_view(e.what()) == "task-error";
        std::this_thread::sleep_for(std::chrono::microseconds(50));
        caught = caught && std::string_view(e.what()) == "task-error";
    }
    worker.join();
    return caught ? 0 : 1;
}

int run_value_once() {
    auto packaged = std::make_shared<std::packaged_task<int()>>([] { return 42; });
    auto future = packaged->get_future();
    std::function<void()> work = [packaged] { (*packaged)(); };
    std::thread worker([work = std::move(work)] mutable {
        work();
    });
    const int value = future.get();
    worker.join();
    return value == 42 ? 0 : 1;
}

int run_exception_join_before_get_once() {
    auto packaged = std::make_shared<std::packaged_task<int()>>(
        []() -> int { throw std::runtime_error("task-error"); });
    auto future = packaged->get_future();
    std::function<void()> work = [packaged] { (*packaged)(); };
    std::thread worker([work = std::move(work)] mutable {
        work();
    });
    worker.join();
    try {
        (void)future.get();
    } catch (const std::runtime_error& e) {
        return std::string_view(e.what()) == "task-error" ? 0 : 1;
    }
    return 1;
}

int main(int argc, char** argv) {
    const std::string_view mode = argc > 1 ? argv[1] : "exception";
    const int rounds = argc > 2 ? std::stoi(argv[2]) : 200;
    for (int i = 0; i < rounds; ++i) {
        int rc = 1;
        if (mode == "exception") rc = run_exception_once();
        else if (mode == "value") rc = run_value_once();
        else if (mode == "join-before-get") rc = run_exception_join_before_get_once();
        else return 2;
        if (rc != 0) return rc;
    }
    std::cout << "ok " << mode << " rounds=" << rounds << '\n';
}
