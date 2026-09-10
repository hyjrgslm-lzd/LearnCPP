#include <cmath>
#include <fmt/args.h>
#include <fmt/compile.h>
#include <fmt/format.h>
#include <functional>
#include <stdexcept>
#include <string>
#include <string_view>

struct SensorReading {
  std::string name;
  int milli;
};

template <>
struct fmt::formatter<SensorReading> {
  constexpr auto parse(fmt::format_parse_context& ctx) { return ctx.begin(); }

  auto format(const SensorReading& reading, fmt::format_context& ctx) const {
    const int whole = reading.milli / 1000;
    const int frac = std::abs(reading.milli % 1000);
    return fmt::format_to(ctx.out(), "{}={}.{:03}", reading.name, whole, frac);
  }
};

std::string fixed_format(fmt::format_string<const SensorReading&> pattern, const SensorReading& reading) {
  return fmt::format(pattern, reading);
}

std::string runtime_format(std::string_view pattern, const SensorReading& reading) {
  try {
    return fmt::format(fmt::runtime(pattern), reading);
  } catch (const fmt::format_error& error) {
    return fmt::format("format_error:{}", error.what());
  }
}

void check(bool condition, std::string_view message) {
  if (!condition) throw std::runtime_error(std::string{message});
}

int main() {
  const SensorReading reading{"temp", 1234};
  check(fixed_format("{}", reading) == "temp=1.234", "fixed formatter failed");
  check(runtime_format("{}", reading) == "temp=1.234", "runtime formatter failed");
  check(runtime_format("{:d}", reading).starts_with("format_error:"), "runtime error path failed");
  check(fmt::format(FMT_COMPILE("{}:{}"), "sensor", 42) == "sensor:42", "FMT_COMPILE path failed");

  std::string copied = "copied";
  char borrowed[] = "Rolling Stones";
  fmt::dynamic_format_arg_store<fmt::format_context> store;
  store.push_back(copied);
  store.push_back(std::cref(borrowed));
  copied = "changed";
  borrowed[9] = 'c';
  const std::string stored = fmt::vformat("{}|{}", store);
  check(stored == "copied|Rolling Scones", fmt::format("dynamic store boundary failed: {}", stored));
}
