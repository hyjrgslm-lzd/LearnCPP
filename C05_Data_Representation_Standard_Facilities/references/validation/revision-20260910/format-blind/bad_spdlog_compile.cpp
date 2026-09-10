#include <fmt/format.h>
#include <spdlog/spdlog.h>
#include <string>

int main() {
  spdlog::logger logger{"bad"};
  const std::string value = "not an int";
  logger.info("{:d}", value);
}
