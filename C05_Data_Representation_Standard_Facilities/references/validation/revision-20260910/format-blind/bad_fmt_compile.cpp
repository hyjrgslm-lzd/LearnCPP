#include <fmt/format.h>
#include <string>

int main() {
  const std::string value = "not an int";
  (void)fmt::format("{:d}", value);
}
