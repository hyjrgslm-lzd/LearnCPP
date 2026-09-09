#include <iostream>

int main()
{
#if defined(__cpp_range_based_for)
    std::cout << "__cpp_range_based_for=" << __cpp_range_based_for << '\n';
#else
    std::cout << "__cpp_range_based_for=not-defined\n";
#endif
    std::cout << "range-for lifetime extension not runtime-probed here\n";
}
