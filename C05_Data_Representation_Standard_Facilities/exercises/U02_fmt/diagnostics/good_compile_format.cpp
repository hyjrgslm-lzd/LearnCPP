#include <fmt/format.h>

int main()
{
    auto text = fmt::format("{:d}", 42);
    return text == "42" ? 0 : 1;
}
