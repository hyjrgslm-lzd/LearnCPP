#pragma once

#include "checks.hpp"

#include <memory>
#include <stdexcept>

struct async_logging_submission {
    static constexpr bool complete = false;

    static std::shared_ptr<spdlog::details::thread_pool> make_pool(std::size_t, std::size_t) {
        throw std::logic_error("TODO Part 1: create the async thread_pool");
    }

    static std::shared_ptr<spdlog::async_logger> make_logger(
        std::shared_ptr<u01::capture_sink>, std::shared_ptr<spdlog::details::thread_pool>,
        spdlog::async_overflow_policy) {
        throw std::logic_error("TODO Part 1: wire async_logger to the pool and policy");
    }

    static void log(const std::shared_ptr<spdlog::async_logger>&, u01::marked_payload) {
        throw std::logic_error("TODO Part 2: submit a formatted async log message");
    }

    static void request_flush(const std::shared_ptr<spdlog::async_logger>&) {
        throw std::logic_error("TODO Part 3: enqueue a flush request");
    }

    static void shutdown(std::shared_ptr<spdlog::async_logger>&,
                         std::shared_ptr<spdlog::details::thread_pool>&) {
        throw std::logic_error("TODO Part 4: stop producers, release logger, then join pool");
    }
};
