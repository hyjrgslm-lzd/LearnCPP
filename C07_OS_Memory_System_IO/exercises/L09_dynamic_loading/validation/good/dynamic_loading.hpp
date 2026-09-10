#pragma once
#include <include/dynamic_loading_contract.hpp>

#include <utility>

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
namespace good_detail {

#ifdef _WIN32
class unique_module {
public:
    explicit unique_module(const std::filesystem::path& path)
        : module_(::LoadLibraryExW(path.wstring().c_str(), nullptr,
              LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32)) {}
    unique_module(const unique_module&) = delete;
    unique_module& operator=(const unique_module&) = delete;
    unique_module(unique_module&& other) noexcept : module_(std::exchange(other.module_, nullptr)) {}
    unique_module& operator=(unique_module&& other) noexcept {
        if (this != &other) reset(std::exchange(other.module_, nullptr));
        return *this;
    }
    ~unique_module() { reset(); }

    explicit operator bool() const { return module_ != nullptr; }
    std::string last_error(std::string_view where) const {
        return std::string{where} + " failed: " + std::to_string(::GetLastError());
    }
    template <class Fn>
    Fn symbol(std::string_view name) const {
        return reinterpret_cast<Fn>(::GetProcAddress(module_, std::string{name}.c_str()));
    }
    bool close() {
        const bool ok = module_ == nullptr || ::FreeLibrary(module_) != 0;
        module_ = nullptr;
        return ok;
    }

private:
    void reset(HMODULE next = nullptr) {
        if (module_) (void)::FreeLibrary(module_);
        module_ = next;
    }

    HMODULE module_ = nullptr;
};
#else
class unique_module {
public:
    explicit unique_module(const std::filesystem::path& path)
        : module_(::dlopen(path.string().c_str(), RTLD_NOW | RTLD_LOCAL)) {}
    unique_module(const unique_module&) = delete;
    unique_module& operator=(const unique_module&) = delete;
    unique_module(unique_module&& other) noexcept : module_(std::exchange(other.module_, nullptr)) {}
    unique_module& operator=(unique_module&& other) noexcept {
        if (this != &other) reset(std::exchange(other.module_, nullptr));
        return *this;
    }
    ~unique_module() { reset(); }

    explicit operator bool() const { return module_ != nullptr; }
    std::string last_error(std::string_view fallback) const {
        const char* text = ::dlerror();
        return text ? text : std::string{fallback};
    }
    template <class Fn>
    Fn symbol(std::string_view name) const {
        (void)::dlerror();
        void* address = ::dlsym(module_, std::string{name}.c_str());
        return ::dlerror() == nullptr ? reinterpret_cast<Fn>(address) : nullptr;
    }
    bool close() {
        const bool ok = module_ == nullptr || ::dlclose(module_) == 0;
        module_ = nullptr;
        return ok;
    }

private:
    void reset(void* next = nullptr) {
        if (module_) (void)::dlclose(module_);
        module_ = next;
    }

    void* module_ = nullptr;
};
#endif

} // namespace good_detail

inline ModuleResult load_module_add(const std::filesystem::path& library, std::string_view symbol,
                                    std::uint32_t a, std::uint32_t b, std::uint32_t nonce) {
    ModuleResult result;
    if (!library.is_absolute()) {
        result.error = "module path must be absolute";
        return result;
    }

    good_detail::unique_module module{library};
    if (!module) {
        result.error = module.last_error("load module");
        return result;
    }

    const auto mix = module.symbol<module_mix_fn>(symbol);
    if (!mix) {
        result.error = module.last_error("resolve symbol");
        result.unloaded = module.close();
        return result;
    }

    result.value = mix(a, b, nonce);
    result.unloaded = module.close();
    result.ok = result.unloaded;
    if (!result.ok) result.error = "module unload failed";
    return result;
}

} // namespace c07_l09
