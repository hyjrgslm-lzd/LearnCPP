#include "check.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#if defined(_WIN32)
#define NOMINMAX
#include <windows.h>
#else
#include <dlfcn.h>
#endif

using value_fn = int (*)();
using init_fn = int (*)();
using record_fn = void (*)(const char*);

namespace {
template <class Fn>
Fn load_symbol(
#if defined(_WIN32)
    HMODULE module,
#else
    void* module,
#endif
    const char* name)
{
#if defined(_WIN32)
    return reinterpret_cast<Fn>(GetProcAddress(module, name));
#else
    return reinterpret_cast<Fn>(dlsym(module, name));
#endif
}
}

int main(int argc, char** argv)
{
    if (argc < 2) {
        std::cerr << "usage: loader_check <library> [symbol] [exit-file]\n";
        return EXIT_FAILURE;
    }

    const char* symbol = argc >= 3 ? argv[2] : "lesson_runtime_value";
    const char* exit_file = argc >= 4 ? argv[3] : nullptr;

#if defined(_WIN32)
    HMODULE module = LoadLibraryA(argv[1]);
#else
    void* module = dlopen(argv[1], RTLD_NOW);
#endif
    if (!module) {
        std::cerr << "missing library: " << argv[1] << '\n';
        return 2;
    }

    auto value = load_symbol<value_fn>(module, symbol);
    if (!value) {
        std::cerr << "missing export: " << symbol << '\n';
#if defined(_WIN32)
        FreeLibrary(module);
#else
        dlclose(module);
#endif
        return 3;
    }

    auto init = load_symbol<init_fn>(module, "lesson_runtime_init_count");
    auto record = load_symbol<record_fn>(module, "lesson_runtime_record_exit_to");
    check(init != nullptr, "loader must find init-count export");
    check(record != nullptr, "loader must find exit-record export");
    check(value() == 7, "loaded function must return 7");
    check(init() == 1, "loaded module must be initialized before function call");
    if (exit_file != nullptr) record(exit_file);

#if defined(_WIN32)
    check(FreeLibrary(module) != 0, "FreeLibrary must release the module reference");
#else
    check(dlclose(module) == 0, "dlclose must release the module reference");
#endif

    if (exit_file != nullptr) {
        std::ifstream input(exit_file);
        std::string line;
        std::getline(input, line);
        check(line == "runtime shutdown observed", "module unload must run recorded shutdown");
    }

    std::cout << "loader check passed\n";
    return EXIT_SUCCESS;
}
