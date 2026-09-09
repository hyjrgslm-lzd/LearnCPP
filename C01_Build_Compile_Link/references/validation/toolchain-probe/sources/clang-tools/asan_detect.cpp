#include <cstdio>

int main() {
    int* value = new int[1]{7};
    value[1] = 42;
    std::printf("%d\n", value[0]);
    delete[] value;
    return 0;
}
