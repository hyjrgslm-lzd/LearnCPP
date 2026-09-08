#include <cstdio>

int main() {
    int values[2] = {1, 2};
    std::printf("%d\n", values[0] + values[1]);
    return values[0] == 1 ? 0 : 1;
}
