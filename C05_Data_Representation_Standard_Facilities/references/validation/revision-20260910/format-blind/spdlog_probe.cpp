#include <functional>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

#if defined(BLIND_USE_STD_BACKEND)
#include <format>
#else
#include <fmt/format.h>
#endif

struct AuditEvent {
  std::string user;
  int code;
};

struct ExplodingEvent {};

#if defined(BLIND_USE_STD_BACKEND)
template <>
struct std::formatter<AuditEvent, char> {
  constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }

  auto format(const AuditEvent& event, std::format_context& ctx) const {
    return std::format_to(ctx.out(), "{}#{}", event.user, event.code);
  }
};

template <>
struct std::formatter<ExplodingEvent, char> {
  constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }

  auto format(const ExplodingEvent&, std::format_context& ctx) const -> std::format_context::iterator {
    throw std::runtime_error("blind formatter boom");
    return ctx.out();
  }
};
#else
template <>
struct fmt::formatter<AuditEvent> {
  constexpr auto parse(fmt::format_parse_context& ctx) { return ctx.begin(); }

  auto format(const AuditEvent& event, fmt::format_context& ctx) const {
    return fmt::format_to(ctx.out(), "{}#{}", event.user, event.code);
  }
};

template <>
struct fmt::formatter<ExplodingEvent> {
  constexpr auto parse(fmt::format_parse_context& ctx) { return ctx.begin(); }

  auto format(const ExplodingEvent&, fmt::format_context& ctx) const -> fmt::format_context::iterator {
    throw std::runtime_error("blind formatter boom");
    return ctx.out();
  }
};
#endif

#include <spdlog/sinks/ostream_sink.h>
#include <spdlog/spdlog.h>

void check(bool condition, std::string_view message) {
  if (!condition) throw std::runtime_error(std::string{message});
}

int main() {
  std::ostringstream out;
  auto sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(out);
  spdlog::logger logger{"blind", sink};
  logger.set_pattern("%v");
  logger.set_level(spdlog::level::info);

  logger.info("{}", AuditEvent{"alice", 403});
  check(out.str().find("alice#403") != std::string::npos, "custom formatter did not reach sink");

  int calls = 0;
  auto side_effect = [&] {
    ++calls;
    return AuditEvent{"debug", 7};
  };

  SPDLOG_LOGGER_DEBUG(&logger, "{}", side_effect());
  logger.debug("{}", side_effect());

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_DEBUG
  check(calls == 2, "active DEBUG macro did not evaluate argument before runtime filter");
#else
  check(calls == 1, "active INFO macro did not prune argument evaluation");
#endif
  check(out.str().find("debug#7") == std::string::npos, "runtime-filtered debug reached sink");

  logger.set_pattern("[%l] %v");
  logger.info("{}", AuditEvent{"bob", 201});
  check(out.str().find("[info] bob#201") != std::string::npos, "runtime pattern output missing");

  std::string handled;
  logger.set_error_handler([&](const std::string& message) { handled = message; });
  logger.info("{}", ExplodingEvent{});
  check(handled.find("blind formatter boom") != std::string::npos, "logger error handler did not receive backend exception");

#if defined(BLIND_USE_STD_BACKEND)
  try {
    (void)std::format("{}", ExplodingEvent{});
    check(false, "direct std backend format did not throw");
  } catch (const std::runtime_error& error) {
    check(std::string{error.what()} == "blind formatter boom", "direct std backend threw wrong error");
  }
#else
  try {
    (void)fmt::format("{}", ExplodingEvent{});
    check(false, "direct fmt backend format did not throw");
  } catch (const std::runtime_error& error) {
    check(std::string{error.what()} == "blind formatter boom", "direct fmt backend threw wrong error");
  }
#endif
}
