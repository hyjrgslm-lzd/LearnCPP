#include <iostream>

int main()
{
#if defined(__clang__)
    std::cout << "compiler=clang\n";
    std::cout << "__clang_major__=" << __clang_major__ << '\n';
    std::cout << "__clang_minor__=" << __clang_minor__ << '\n';
    std::cout << "__clang_patchlevel__=" << __clang_patchlevel__ << '\n';
#if defined(_MSC_VER)
    std::cout << "_MSC_VER=" << _MSC_VER << '\n';
#endif
#elif defined(_MSC_VER)
    std::cout << "compiler=msvc\n";
    std::cout << "_MSC_VER=" << _MSC_VER << '\n';
#else
    std::cout << "compiler=other\n";
#endif

    std::cout << "__cplusplus=" << __cplusplus << '\n';
#if defined(_MSVC_LANG)
    std::cout << "_MSVC_LANG=" << _MSVC_LANG << '\n';
#else
    std::cout << "_MSVC_LANG=not-defined\n";
#endif

#if defined(__cpp_implicit_move)
    std::cout << "__cpp_implicit_move=" << __cpp_implicit_move << '\n';
#else
    std::cout << "__cpp_implicit_move=not-defined\n";
#endif

#if defined(__cpp_range_based_for)
    std::cout << "__cpp_range_based_for=" << __cpp_range_based_for << '\n';
#else
    std::cout << "__cpp_range_based_for=not-defined\n";
#endif

#if defined(__cpp_lib_start_lifetime_as)
    std::cout << "__cpp_lib_start_lifetime_as=" << __cpp_lib_start_lifetime_as << '\n';
#else
    std::cout << "__cpp_lib_start_lifetime_as=not-defined\n";
#endif
}
