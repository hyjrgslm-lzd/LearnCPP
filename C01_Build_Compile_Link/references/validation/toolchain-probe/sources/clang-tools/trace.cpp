#include <algorithm>
#include <array>

int main() {
    std::array<int, 4> values{4, 1, 3, 2};
    std::ranges::sort(values);
    return values[0] == 1 && values[3] == 4 ? 0 : 1;
}
