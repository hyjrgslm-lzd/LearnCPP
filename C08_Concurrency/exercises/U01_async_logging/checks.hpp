#pragma once

#include "concurrency_study/exercise_check.hpp"

#include <spdlog/async_logger.h>
#include <spdlog/common.h>
#include <spdlog/details/thread_pool.h>
#include <spdlog/sinks/sink.h>

#include <atomic>
#include <chrono>
#include <exception>
#include <fmt/format.h>
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

namespace u01 {
using namespace std::chrono_literals;

struct marked_payload {
    std::string text;
    std::thread::id* formatted_on{};
};
} // namespace u01

template<>
struct fmt::formatter<u01::marked_payload> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    auto format(const u01::marked_payload& payload, format_context& ctx) const {
        if (payload.formatted_on) *payload.formatted_on = std::this_thread::get_id();
        return fmt::format_to(ctx.out(), "{}", payload.text);
    }
};

namespace u01 {

class manual_gate {
public:
    std::shared_future<void> token() { return ready_.get_future().share(); }
    void open() {
        bool expected = false;
        if (opened_.compare_exchange_strong(expected, true)) ready_.set_value();
    }
private:
    std::promise<void> ready_;
    std::atomic<bool> opened_{};
};

class gate_guard {
public:
    explicit gate_guard(manual_gate& gate) : gate_(&gate) {}
    gate_guard(const gate_guard&) = delete;
    gate_guard& operator=(const gate_guard&) = delete;
    ~gate_guard() { if (gate_) gate_->open(); }
    void release_now() {
        if (gate_) {
            gate_->open();
            gate_ = nullptr;
        }
    }
private:
    manual_gate* gate_;
};

class capture_sink final : public spdlog::sinks::sink {
public:
    void block_first_on(std::string text, std::shared_future<void> gate) {
        block_text_ = std::move(text);
        gate_ = std::move(gate);
    }
    std::future<void> first_blocked() { return first_blocked_.get_future(); }
    void throw_on(std::string text) { throw_text_ = std::move(text); }
    std::thread::id worker_thread() const {
        std::lock_guard lock(mutex_);
        return worker_thread_;
    }

    void log(const spdlog::details::log_msg& msg) override {
        {
            std::lock_guard lock(mutex_);
            worker_thread_ = std::this_thread::get_id();
        }
        std::string text{msg.payload.data(), msg.payload.size()};
        if (throw_text_ && text == *throw_text_) {
            throw spdlog::spdlog_ex("sink failed: " + text);
        }
        if (block_text_ && text == *block_text_) {
            first_blocked_.set_value();
            gate_.wait();
        }
        std::lock_guard lock(mutex_);
        messages_.push_back(std::move(text));
    }
    void flush() override {
        std::lock_guard lock(mutex_);
        ++flushes_;
    }
    void set_pattern(const std::string&) override {}
    void set_formatter(std::unique_ptr<spdlog::formatter>) override {}

    std::vector<std::string> messages() const {
        std::lock_guard lock(mutex_);
        return messages_;
    }
    int flushes() const {
        std::lock_guard lock(mutex_);
        return flushes_;
    }

private:
    mutable std::mutex mutex_;
    std::vector<std::string> messages_;
    int flushes_{};
    std::optional<std::string> block_text_;
    std::optional<std::string> throw_text_;
    std::shared_future<void> gate_;
    std::promise<void> first_blocked_;
    std::thread::id worker_thread_{};
};

inline void wait_until(bool (*predicate)(void*), void* context, std::string_view message) {
    const auto deadline = std::chrono::steady_clock::now() + 2s;
    while (!predicate(context) && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::yield();
    }
    cs::check(predicate(context), message);
}

template<class Submission>
std::vector<std::string> run_blocked_capacity_one(spdlog::async_overflow_policy policy,
                                                  std::size_t& overruns,
                                                  std::size_t& discards) {
    auto sink = std::make_shared<capture_sink>();
    auto pool = Submission::make_pool(1, 1);
    manual_gate gate;
    gate_guard guard(gate);
    sink->block_first_on("first", gate.token());
    auto blocked = sink->first_blocked();
    auto logger = Submission::make_logger(sink, pool, policy);
    Submission::log(logger, marked_payload{"first", nullptr});
    cs::check(blocked.wait_for(2s) == std::future_status::ready, "worker reached controlled sink");
    Submission::log(logger, marked_payload{"second", nullptr});
    Submission::log(logger, marked_payload{"third", nullptr});
    overruns = pool->overrun_counter();
    discards = pool->discard_counter();
    guard.release_now();
    Submission::shutdown(logger, pool);
    return sink->messages();
}

template<class Submission>
void check_overflow_policies() {
    std::size_t overruns = 0;
    std::size_t discards = 0;
    auto overrun = run_blocked_capacity_one<Submission>(
        spdlog::async_overflow_policy::overrun_oldest, overruns, discards);
    cs::check(overruns == 1 && discards == 0, "overrun counter");
    cs::check((overrun == std::vector<std::string>{"first", "third"}),
              "overrun_oldest keeps newest queued message");

    auto discard = run_blocked_capacity_one<Submission>(
        spdlog::async_overflow_policy::discard_new, overruns, discards);
    cs::check(overruns == 0 && discards == 1, "discard counter");
    cs::check((discard == std::vector<std::string>{"first", "second"}),
              "discard_new keeps existing queued message");
}

template<class Submission>
void check_blocking_policy() {
    auto sink = std::make_shared<capture_sink>();
    auto pool = Submission::make_pool(1, 1);
    manual_gate gate;
    gate_guard guard(gate);
    sink->block_first_on("first", gate.token());
    auto blocked = sink->first_blocked();
    auto logger = Submission::make_logger(sink, pool, spdlog::async_overflow_policy::block);
    Submission::log(logger, marked_payload{"first", nullptr});
    cs::check(blocked.wait_for(2s) == std::future_status::ready, "block setup");
    Submission::log(logger, marked_payload{"second", nullptr});
    auto third = std::async(std::launch::async, [logger] {
        Submission::log(logger, marked_payload{"third", nullptr});
    });
    if (third.wait_for(100ms) == std::future_status::ready) {
        third.get();
        cs::check(false, "block waits for queue room");
    }
    guard.release_now();
    third.get();
    Submission::shutdown(logger, pool);
    cs::check((sink->messages() == std::vector<std::string>{"first", "second", "third"}),
              "block drains FIFO");
}

template<class Submission>
void check_single_and_multi_worker_order() {
    auto single_sink = std::make_shared<capture_sink>();
    auto single_pool = Submission::make_pool(8, 1);
    auto single = Submission::make_logger(single_sink, single_pool, spdlog::async_overflow_policy::block);
    for (int i = 0; i != 4; ++i) Submission::log(single, marked_payload{std::to_string(i), nullptr});
    Submission::shutdown(single, single_pool);
    cs::check((single_sink->messages() == std::vector<std::string>{"0", "1", "2", "3"}),
              "one worker preserves FIFO completion");

    auto multi_sink = std::make_shared<capture_sink>();
    auto multi_pool = Submission::make_pool(8, 2);
    manual_gate gate;
    gate_guard guard(gate);
    multi_sink->block_first_on("slow", gate.token());
    auto blocked = multi_sink->first_blocked();
    auto multi = Submission::make_logger(multi_sink, multi_pool, spdlog::async_overflow_policy::block);
    Submission::log(multi, marked_payload{"slow", nullptr});
    cs::check(blocked.wait_for(2s) == std::future_status::ready, "multi worker slow entered");
    Submission::log(multi, marked_payload{"fast", nullptr});
    auto has_fast = [](void* ctx) {
        return static_cast<capture_sink*>(ctx)->messages() == std::vector<std::string>{"fast"};
    };
    wait_until(has_fast, multi_sink.get(), "two workers may complete out of enqueue order");
    guard.release_now();
    Submission::shutdown(multi, multi_pool);
}

template<class Submission>
void check_payload_format_thread_and_lifetime() {
    auto sink = std::make_shared<capture_sink>();
    auto pool = Submission::make_pool(4, 1);
    manual_gate gate;
    gate_guard guard(gate);
    sink->block_first_on("owned", gate.token());
    auto blocked = sink->first_blocked();
    auto logger = Submission::make_logger(sink, pool, spdlog::async_overflow_policy::block);
    std::thread::id formatted_on{};
    std::string payload = "owned";
    Submission::log(logger, marked_payload{payload, &formatted_on});
    payload = "mutated";
    cs::check(formatted_on == std::this_thread::get_id(), "payload formats on caller thread");
    cs::check(blocked.wait_for(2s) == std::future_status::ready, "payload held before drain");
    cs::check(sink->worker_thread() != std::this_thread::get_id(), "sink runs on worker thread");
    guard.release_now();
    Submission::shutdown(logger, pool);
    cs::check((sink->messages() == std::vector<std::string>{"owned"}),
              "async message owns formatted payload");
}

template<class Submission>
void check_errors_flush_and_lifetime() {
    auto sink = std::make_shared<capture_sink>();
    auto pool = Submission::make_pool(2, 1);
    auto logger = Submission::make_logger(sink, pool, spdlog::async_overflow_policy::block);
    sink->throw_on("boom");
    std::atomic<int> handled{};
    logger->set_error_handler([&](const std::string& msg) {
        if (msg.find("sink failed: boom") != std::string::npos) ++handled;
    });
    Submission::log(logger, marked_payload{"boom", nullptr});
    Submission::shutdown(logger, pool);
    cs::check(handled == 1, "sink exception reaches error_handler");

    sink = std::make_shared<capture_sink>();
    pool = Submission::make_pool(1, 1);
    manual_gate gate;
    gate_guard guard(gate);
    sink->block_first_on("held", gate.token());
    auto blocked = sink->first_blocked();
    logger = Submission::make_logger(sink, pool, spdlog::async_overflow_policy::block);
    Submission::log(logger, marked_payload{"held", nullptr});
    cs::check(blocked.wait_for(2s) == std::future_status::ready, "flush setup");
    Submission::request_flush(logger);
    cs::check(sink->flushes() == 0, "flush request is not completion");
    guard.release_now();
    Submission::shutdown(logger, pool);
    cs::check(sink->flushes() == 1, "queued flush runs during drain");

    sink = std::make_shared<capture_sink>();
    pool = Submission::make_pool(1, 1);
    manual_gate discard_gate;
    gate_guard discard_guard(discard_gate);
    sink->block_first_on("held", discard_gate.token());
    blocked = sink->first_blocked();
    logger = Submission::make_logger(sink, pool, spdlog::async_overflow_policy::discard_new);
    Submission::log(logger, marked_payload{"held", nullptr});
    cs::check(blocked.wait_for(2s) == std::future_status::ready, "discard flush setup");
    Submission::log(logger, marked_payload{"queued", nullptr});
    Submission::request_flush(logger);
    cs::check(pool->discard_counter() == 1, "full queue may discard flush request");
    discard_guard.release_now();
    Submission::shutdown(logger, pool);
    cs::check(sink->flushes() == 0, "discarded flush has no completion effect");

    sink = std::make_shared<capture_sink>();
    pool = Submission::make_pool(1, 1);
    logger = Submission::make_logger(sink, pool, spdlog::async_overflow_policy::block);
    pool.reset();
    handled = 0;
    logger->set_error_handler([&](const std::string& msg) {
        if (msg.find("thread pool doesn't exist") != std::string::npos) ++handled;
    });
    Submission::log(logger, marked_payload{"after-pool", nullptr});
    Submission::request_flush(logger);
    cs::check(handled == 2, "expired thread pool reports log and flush errors");
}

template<class Submission>
void run_all() {
    check_blocking_policy<Submission>();
    check_overflow_policies<Submission>();
    check_single_and_multi_worker_order<Submission>();
    check_payload_format_thread_and_lifetime<Submission>();
    check_errors_flush_and_lifetime<Submission>();
}
} // namespace u01
