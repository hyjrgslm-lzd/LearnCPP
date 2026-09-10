#include <check.hpp>

#include <array>
#include <format>

int main()
{
    std::array<char, 4> buffer{};
    auto result = std::format_to_n(buffer.begin(), buffer.size(), "id={}", 123456);
    check(result.out == buffer.end(), "bounded write stays in buffer");
    check(static_cast<std::size_t>(result.size) <= buffer.size(), "format_to_n reports truncation");
}
