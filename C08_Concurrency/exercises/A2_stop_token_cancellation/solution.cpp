#include "concurrency_study/exercise_check.hpp"
#include <atomic>
#include <future>
#include <iostream>
#include <memory>
#include <stop_token>
#include <thread>

void polling() {
    std::promise<void> started;
    auto ready = started.get_future();
    std::exception_ptr error;
    bool observed_stop = false;
    std::jthread worker([&](std::stop_token token) {
        try {
            started.set_value();
            while (!token.stop_requested()) std::this_thread::yield();
            observed_stop = true;
        } catch (...) { error = std::current_exception(); }
    });
    ready.get();
    const bool first = worker.request_stop();
    const bool repeated = worker.request_stop();
    worker.join();
    if (error) std::rethrow_exception(error);
    cs::check(first && !repeated && observed_stop, "jthread observes actual stop request");
}

void sources_and_callbacks() {
    std::stop_source source;
    auto copy = source;
    const auto token = source.get_token();
    int calls = 0;
    std::thread::id callback_thread;
    std::stop_callback callback(token, [&]() noexcept {
        ++calls;
        callback_thread = std::this_thread::get_id();
    });
    const auto requester = std::this_thread::get_id();
    cs::check(copy.request_stop(), "copied source requests shared state");
    cs::check(source.stop_requested() && token.stop_requested(), "sources and token share stop");
    cs::check(calls == 1 && callback_thread == requester, "registered callback runs synchronously");
    std::stop_callback late(token, [&]() noexcept { ++calls; });
    cs::check(calls == 2 && !source.request_stop(), "late registration executes before constructor returns");
    std::stop_token orphan;
    { std::stop_source temporary; orphan = temporary.get_token(); }
    cs::check(!orphan.stop_possible() && !orphan.stop_requested(), "no remaining source and no request");
    cs::check(!std::stop_token{}.stop_possible(), "default token cannot stop");

    std::stop_source external;
    auto watcher = std::async(std::launch::async, [t = external.get_token()] {
        while (!t.stop_requested()) std::this_thread::yield();
        return t.stop_requested();
    });
    external.request_stop();
    cs::check(watcher.get(), "external source independent of jthread");
}

void callback_destruction() {
    std::stop_source source;
    std::promise<void> entered, release;
    auto entry = entered.get_future();
    auto leave = release.get_future().share();
    std::atomic<bool> completed{false};
    auto fn = [&]() noexcept {
        entered.set_value();
        leave.wait();
        completed.store(true);
    };
    auto callback = std::make_unique<std::stop_callback<decltype(fn)>>(source.get_token(), fn);
    auto request = std::async(std::launch::async, [&] { return source.request_stop(); });
    entry.get(); // callback is now running on another thread
    std::future<bool> destroy;
    try {
        destroy = std::async(std::launch::async, [&] {
            callback.reset();
            return completed.load(); // must observe completed callback after deregistration
        });
    } catch (...) {
        release.set_value(); // let request's future destructor finish on launch failure
        throw;
    }
    release.set_value();
    cs::check(request.get() && destroy.get(), "callback destructor waits for concurrent callback");
}

int main() {
    polling(); sources_and_callbacks(); callback_destruction();
#if CS_HAS_INPLACE_STOP_TOKEN
    // C++26 extension only when the build actually probed these library types.
    std::inplace_stop_source source;
    auto token = source.get_token();
    int calls = 0;
    std::inplace_stop_callback callback(token, [&]() noexcept { ++calls; });
    cs::check(source.request_stop() && token.stop_requested() && calls == 1, "inplace stop");
#else
    std::cout << "SKIP inplace_stop_token: capability unavailable; C++23 baseline checked\n";
#endif
    std::cout << "A2 OK: polling, shared state, callbacks, callback lifetime\n";
}
