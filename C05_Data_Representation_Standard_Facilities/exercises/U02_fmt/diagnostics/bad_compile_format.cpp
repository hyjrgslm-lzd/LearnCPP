#include <fmt/format.h>

int main()
{
    auto text = fmt::format("{:d}", "not an integer");
    return text.empty() ? 1 : 0;
}
