#include <check.hpp>

#include <spdlog/sinks/ostream_sink.h>
#include <spdlog/spdlog.h>

#include <format>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

struct Reading {
    std::string name;
    int percent{};
};

struct Exploding {};

#ifdef SPDLOG_USE_STD_FORMAT
template<>
struct std::formatter<Reading> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    auto format(const Reading& reading, format_context& ctx) const
    {
        return std::format_to(ctx.out(), "{}={}%", reading.name, reading.percent);
    }
};

template<>
struct std::formatter<Exploding> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    auto format(const Exploding&, format_context& ctx) const
    {
        throw std::runtime_error("backend formatter exploded");
        return ctx.out();
    }
};
#else
template<>
struct fmt::formatter<Reading> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    auto format(const Reading& reading, format_context& ctx) const
    {
        return fmt::format_to(ctx.out(), "{}={}%", reading.name, reading.percent);
    }
};

template<>
struct fmt::formatter<Exploding> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    auto format(const Exploding&, format_context& ctx) const
    {
        throw std::runtime_error("backend formatter exploded");
        return ctx.out();
    }
};
#endif

static int calls{};

static int counted()
{
    ++calls;
    return 7;
}

static bool backend_rejects_runtime_format()
{
    try {
#ifdef SPDLOG_USE_STD_FORMAT
        (void)std::vformat("{:d}", std::make_format_args("text"));
    } catch (const std::format_error&) {
        return true;
#else
        (void)fmt::format(fmt::runtime("{:d}"), "text");
    } catch (const fmt::format_error&) {
        return true;
#endif
    }
    return false;
}

int main()
{
    std::ostringstream out;
    auto sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(out);
    spdlog::logger logger{"lesson", sink};
    logger.set_level(spdlog::level::info);
    logger.set_pattern("[%l] %v");

    logger.info("{}", Reading{"cpu", 85});
    check(out.str().find("[info] cpu=85%") != std::string::npos,
        "custom formatter passes through spdlog backend");
    check(out.str().find("[info]") != std::string::npos,
        "pattern_formatter uses runtime logger configuration");

    calls = 0;
    SPDLOG_LOGGER_DEBUG(&logger, "debug {}", counted());
    const int active_macro_calls = calls;
#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_DEBUG
    check(active_macro_calls == 1, "active debug macro evaluates arguments before runtime filtering");
#else
    check(active_macro_calls == 0, "inactive debug macro removes argument evaluation");
#endif
    check(out.str().find("debug 7") == std::string::npos,
        "runtime level filter prevents sink write");

    calls = 0;
    logger.debug("function debug {}", counted());
    const int member_debug_calls = calls;
    check(member_debug_calls == 1, "ordinary debug member call still evaluates arguments");
    check(out.str().find("function debug") == std::string::npos,
        "ordinary member call also stops before sink when level filters it");

    check(backend_rejects_runtime_format(), "backend reports runtime format errors");

    int handled = 0;
    logger.set_error_handler([&handled](const std::string& msg) {
        if (msg.find("backend formatter exploded") != std::string::npos) {
            ++handled;
        }
    });
    logger.info("{}", Exploding{});
    check(handled == 1, "logger error_handler observes backend formatter exception");

    std::cout << "backend=" << std::string_view{C05_SPDLOG_BACKEND} << '\n';
    std::cout << "active_level=" << SPDLOG_ACTIVE_LEVEL << '\n';
    std::cout << "active_macro_calls=" << active_macro_calls << '\n';
    std::cout << "member_debug_calls=" << member_debug_calls << '\n';
    std::cout << "backend_runtime_error=" << backend_rejects_runtime_format() << '\n';
    std::cout << "logger_error_handler_calls=" << handled << '\n';
    std::cout << "sink_text=" << out.str() << '\n';

#ifdef C05_SPDLOG_SOURCE_COMMIT
    check(std::string_view{C05_SPDLOG_SOURCE_COMMIT}
            == "79524ddd08a4ec981b7fea76afd08ee05f83755d",
        "spdlog source commit is pinned");
#endif
#ifdef C05_SPDLOG_BACKEND
    check(std::string_view{C05_SPDLOG_BACKEND} == "fmt"
            || std::string_view{C05_SPDLOG_BACKEND} == "std",
        "spdlog backend is explicit");
#endif
}
