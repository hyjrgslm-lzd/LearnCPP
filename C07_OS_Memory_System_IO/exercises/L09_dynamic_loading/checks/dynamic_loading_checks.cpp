#include <dynamic_loading.hpp>
#include <include/dynamic_loading_contract.hpp>
#include <check.hpp>

#include <bit>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <dlfcn.h>
#include <unistd.h>
#endif

namespace {

std::uint32_t expected_mix(std::uint32_t a, std::uint32_t b, std::uint32_t nonce) {
    return std::rotl(a ^ nonce, 5) ^ (b * 2654435761u) ^ 0xC07D9009u;
}

class module_observer {
public:
    explicit module_observer(const std::filesystem::path& path) {
#ifdef _WIN32
        handle_ = ::LoadLibraryExW(path.wstring().c_str(), nullptr,
            LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32);
        check(handle_ != nullptr, "observer loads fixture module");
        auto* proc = ::GetProcAddress(static_cast<HMODULE>(handle_), c07_l09::module_calls_symbol);
        check(proc != nullptr, "observer resolves call counter");
        calls_ = reinterpret_cast<c07_l09::module_calls_fn>(proc);
#else
        handle_ = ::dlopen(path.string().c_str(), RTLD_NOW | RTLD_LOCAL);
        check(handle_ != nullptr, "observer loads fixture module");
        (void)::dlerror();
        auto* proc = ::dlsym(handle_, c07_l09::module_calls_symbol);
        check(::dlerror() == nullptr, "observer resolves call counter");
        calls_ = reinterpret_cast<c07_l09::module_calls_fn>(proc);
#endif
    }

    module_observer(const module_observer&) = delete;
    module_observer& operator=(const module_observer&) = delete;

    ~module_observer() {
#ifdef _WIN32
        if (handle_) (void)::FreeLibrary(static_cast<HMODULE>(handle_));
#else
        if (handle_) (void)::dlclose(handle_);
#endif
    }

    std::uint32_t calls() const { return calls_(); }

private:
    void* handle_ = nullptr;
    c07_l09::module_calls_fn calls_ = nullptr;
};

std::uint32_t runtime_nonce() {
    const auto ticks = static_cast<std::uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count());
#ifdef _WIN32
    const auto pid = static_cast<std::uint64_t>(::GetCurrentProcessId());
#else
    const auto pid = static_cast<std::uint64_t>(::getpid());
#endif
    return static_cast<std::uint32_t>((ticks >> 7u) ^ (ticks >> 32u) ^ pid ^ 0x9E37u);
}

} // namespace

int main(int argc, char** argv) {
    check(argc == 2, "module path argument is provided");
    const auto module = std::filesystem::absolute(argv[1]);
    module_observer observer{module};

    const std::uint32_t a = 0x8123'4567u;
    const std::uint32_t b = 0x0001'00F3u;
    const std::uint32_t nonce = runtime_nonce();

    const auto before = observer.calls();
    const auto loaded = c07_l09::load_module_add(module, c07_l09::module_symbol, a, b, nonce);
    const auto after = observer.calls();
    check(loaded.ok, loaded.error.empty() ? "module loads" : loaded.error);
    check(after == before + 1, "module call count changed once");
    check(loaded.value == expected_mix(a, b, nonce), "module result matches exported C ABI");

    const auto missing_symbol_before = observer.calls();
    const auto missing_symbol = c07_l09::load_module_add(module, "c07_module_missing_symbol", a, b, nonce + 1u);
    check(!missing_symbol.ok, "missing symbol is rejected");
    check(observer.calls() == missing_symbol_before, "missing symbol does not call module");

    const auto missing_library = c07_l09::load_module_add(module.parent_path() / "missing-c07-module",
        c07_l09::module_symbol, a, b, nonce + 2u);
    check(!missing_library.ok, "missing library is rejected");

    const auto relative = c07_l09::load_module_add(module.filename(), c07_l09::module_symbol, a, b, nonce + 3u);
    check(!relative.ok, "relative module path is rejected");

    std::cout << "L09 dynamic loading checks passed\n";
}
