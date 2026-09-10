#pragma once
#include <include/dynamic_loading_contract.hpp>

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
#endif

namespace c07_l09 {

inline ModuleResult load_module_add(const std::filesystem::path& library, std::string_view symbol,
                                    std::uint32_t a, std::uint32_t b, std::uint32_t nonce) {
    ModuleResult result;
    if (!library.is_absolute()) {
        result.error = "module path must be absolute";
        return result;
    }
    const std::string symbol_name{symbol};

#ifdef _WIN32
    HMODULE module = ::LoadLibraryExW(library.wstring().c_str(), nullptr,
        LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!module) {
        result.error = "LoadLibraryExW failed: " + std::to_string(::GetLastError());
        return result;
    }
    auto cleanup = [&] {
        if (module) {
            result.unloaded = ::FreeLibrary(module) != 0;
            module = nullptr;
        }
    };
    FARPROC proc = ::GetProcAddress(module, symbol_name.c_str());
    if (!proc) {
        result.error = "GetProcAddress failed: " + std::to_string(::GetLastError());
        cleanup();
        return result;
    }
    auto fn = reinterpret_cast<module_mix_fn>(proc);
#else
    void* module = ::dlopen(library.string().c_str(), RTLD_NOW | RTLD_LOCAL);
    if (!module) {
        const char* error = ::dlerror();
        result.error = error ? error : "dlopen failed";
        return result;
    }
    auto cleanup = [&] {
        if (module) {
            result.unloaded = ::dlclose(module) == 0;
            module = nullptr;
        }
    };
    (void)::dlerror();
    void* proc = ::dlsym(module, symbol_name.c_str());
    const char* symbol_error = ::dlerror();
    if (symbol_error) {
        result.error = symbol_error;
        cleanup();
        return result;
    }
    auto fn = reinterpret_cast<module_mix_fn>(proc);
#endif

    result.value = fn(a, b, nonce);
    result.ok = true;
    cleanup();
    if (!result.unloaded) {
        result.ok = false;
        result.error = "module unload failed";
    }
    return result;
}

} // namespace c07_l09
