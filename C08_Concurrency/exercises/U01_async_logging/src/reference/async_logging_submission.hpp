#pragma once

#include "checks.hpp"

#include <memory>
#include <string>
#include <utility>

struct async_logging_submission {
    static constexpr bool complete = true;

    static std::shared_ptr<spdlog::details::thread_pool> make_pool(std::size_t queue_size,
                                                                   std::size_t workers) {
        return std::make_shared<spdlog::details::thread_pool>(queue_size, workers);
    }

    static std::shared_ptr<spdlog::async_logger> make_logger(
        std::shared_ptr<u01::capture_sink> sink,
        std::shared_ptr<spdlog::details::thread_pool> pool,
        spdlog::async_overflow_policy policy) {
        auto logger = std::make_shared<spdlog::async_logger>(
            "u01-reference", std::move(sink), std::move(pool), policy);
        logger->set_level(spdlog::level::info);
        return logger;
    }

    static void log(const std::shared_ptr<spdlog::async_logger>& logger, u01::marked_payload payload) {
        logger->info("{}", payload);
    }

    static void request_flush(const std::shared_ptr<spdlog::async_logger>& logger) {
        logger->flush();
    }

    static void shutdown(std::shared_ptr<spdlog::async_logger>& logger,
                         std::shared_ptr<spdlog::details::thread_pool>& pool) {
        logger.reset();
        pool.reset();
    }
};
