#include <cstddef>
int main() {
    return sizeof(std::size_t) > 0 ? 0 : 1;
}
