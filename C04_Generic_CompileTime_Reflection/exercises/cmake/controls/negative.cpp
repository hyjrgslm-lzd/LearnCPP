#include <type_traits>
static_assert(!std::is_integral_v<int>, "c04_compile_control");
int main() { return 0; }
