#pragma once
#include "c18/abi.h"
#include <filesystem>
#include <span>
#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace c18 {
// P1's serial baseline: no callbacks, escaped function pointers, or concurrent calls.
// The concurrent shutdown protocol is the separate L04 learning task.
class SerialPlugin {
public:
    SerialPlugin() = default;
    SerialPlugin(const SerialPlugin&) = delete;
    SerialPlugin& operator=(const SerialPlugin&) = delete;
    ~SerialPlugin() { (void)close(); }

    c18_status open(const std::filesystem::path& path) {
        if (module_ || !path.is_absolute()) return C18_STATUS_BAD_ARGUMENT;
#ifdef _WIN32
        module_ = LoadLibraryExW(path.c_str(), nullptr, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32);
        if (!module_) return C18_STATUS_PLUGIN_ERROR;
        auto query = reinterpret_cast<c18_get_api_fn>(GetProcAddress(module_, "c18_get_api"));
#else
        module_ = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
        if (!module_) return C18_STATUS_PLUGIN_ERROR;
        dlerror();
        auto query = reinterpret_cast<c18_get_api_fn>(dlsym(module_, "c18_get_api"));
        if (dlerror()) query = nullptr;
#endif
        if (!query) return fail_open(C18_STATUS_MISSING_SYMBOL);
        // A catch here only protects this C++ host against a same-toolchain faulty fixture.
        // It is not permission for production plugins to throw through the C ABI.
        try {
            auto status = query(C18_ABI_VERSION, sizeof(api_), &api_);
            if (status != C18_STATUS_OK) return fail_open(status);
            if (api_.version != C18_ABI_VERSION || api_.struct_size < sizeof(api_) ||
                !api_.create || !api_.process || !api_.request_stop || !api_.destroy) {
                return fail_open(C18_STATUS_UNSUPPORTED_VERSION);
            }
            c18_host_api host{C18_ABI_VERSION, sizeof(c18_host_api), nullptr, nullptr};
            status = api_.create(&host, &context_);
            if (status != C18_STATUS_OK || !context_) {
                // A compliant failed create leaves no owned state.
                return fail_open(status == C18_STATUS_OK ? C18_STATUS_PLUGIN_ERROR : status);
            }
            stopping_ = false;
            return C18_STATUS_OK;
        } catch (...) {
            return fail_open(C18_STATUS_PLUGIN_ERROR);
        }
    }

    c18_status process(std::span<const uint8_t> input, std::span<uint8_t> output, size_t& written) noexcept {
        written = 0;
        if (!context_ || stopping_) return C18_STATUS_CLOSING;
        try { return api_.process(context_, input.data(), input.size(), output.data(), output.size(), &written); }
        catch (...) { return C18_STATUS_PLUGIN_ERROR; }
    }

    c18_status close() noexcept {
        stopping_ = true;
        if (context_) {
            try {
                auto status = api_.request_stop(context_);
                if (status != C18_STATUS_OK) return status;
                status = api_.destroy(context_);
                if (status != C18_STATUS_OK) return status;
                context_ = nullptr;
            } catch (...) { return C18_STATUS_PLUGIN_ERROR; }
        }
        return release_module();
    }

private:
    c18_status fail_open(c18_status cause) noexcept {
        if (!context_ && release_module() != C18_STATUS_OK) return C18_STATUS_PLUGIN_ERROR;
        return cause;
    }
    c18_status release_module() noexcept {
        if (!module_) return C18_STATUS_OK;
#ifdef _WIN32
        if (!FreeLibrary(module_)) return C18_STATUS_PLUGIN_ERROR;
#else
        if (dlclose(module_) != 0) return C18_STATUS_PLUGIN_ERROR;
#endif
        module_ = nullptr;
        api_ = {};
        return C18_STATUS_OK;
    }
#ifdef _WIN32
    HMODULE module_{};
#else
    void* module_{};
#endif
    c18_context* context_{};
    c18_api api_{};
    bool stopping_{};
};
}
