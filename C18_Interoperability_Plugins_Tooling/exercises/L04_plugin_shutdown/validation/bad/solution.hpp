#pragma once

#include "c18/abi.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <mutex>
#include <string_view>
#include <utility>
#include <vector>

#ifdef _WIN32
#  include <windows.h>
#else
#  include <dlfcn.h>
#endif

namespace c18_l04 {

struct ProcessResult {
  c18_status status{C18_STATUS_PLUGIN_ERROR};
  std::vector<uint8_t> bytes{};
  size_t required{};
};

class PluginSession {
public:
  PluginSession() = default;
  PluginSession(const PluginSession&) = delete;
  auto operator=(const PluginSession&) -> PluginSession& = delete;
  ~PluginSession() { (void)close(); }

  auto open(const std::filesystem::path& path) -> c18_status {
    handle_ = load(path);
    if (handle_ == nullptr) {
      return C18_STATUS_PLUGIN_ERROR;
    }
    auto* symbol = sym(handle_, "c18_get_api");
    if (symbol == nullptr) {
      unload(handle_);
      handle_ = nullptr;
      return C18_STATUS_MISSING_SYMBOL;
    }
    auto* entry = reinterpret_cast<c18_get_api_fn>(symbol);
    c18_status status = entry(C18_ABI_VERSION, sizeof(api_), &api_);
    if (status != C18_STATUS_OK || api_.version != C18_ABI_VERSION || api_.struct_size < sizeof(c18_api) ||
        api_.create == nullptr || api_.process == nullptr || api_.request_stop == nullptr ||
        api_.destroy == nullptr) {
      unload(handle_);
      handle_ = nullptr;
      return status == C18_STATUS_OK ? static_cast<c18_status>(C18_STATUS_UNSUPPORTED_VERSION) : status;
    }
    c18_host_api host{C18_ABI_VERSION, sizeof(c18_host_api), this, &PluginSession::host_event};
    status = api_.create(&host, &ctx_);
    if (status != C18_STATUS_OK || ctx_ == nullptr) {
      unload(handle_);
      handle_ = nullptr;
      return status == C18_STATUS_OK ? static_cast<c18_status>(C18_STATUS_PLUGIN_ERROR) : status;
    }
    running_ = true;
    return C18_STATUS_OK;
  }

  auto set_callback(std::function<void(std::string_view)> callback) -> void {
    std::scoped_lock guard(lock_);
    callback_ = std::move(callback);
  }

  auto set_observer(std::function<void(std::string_view)> observer) -> void {
    std::scoped_lock guard(lock_);
    observer_ = std::move(observer);
  }

  auto fail_next_release_for_test() -> void { fail_next_release_ = true; }

  auto process(const std::vector<uint8_t>& input, size_t capacity) -> ProcessResult {
    if (!running_) {
      return {C18_STATUS_CLOSING, {}, input.size()};
    }
    ProcessResult result;
    result.bytes.assign(capacity, 0xCCu);
    size_t written = 0;
    result.status = api_.process(ctx_, input.data(), input.size(), result.bytes.data(), result.bytes.size(), &written);
    result.required = written;
    if (result.status == C18_STATUS_OK) {
      result.bytes.resize(written);
    }
    return result;
  }

  auto request_stop() -> c18_status {
    running_ = false;
    if (ctx_ == nullptr) {
      return C18_STATUS_CLOSING;
    }
    return api_.request_stop(ctx_);
  }

  auto close() -> c18_status {
    if (ctx_ == nullptr && handle_ == nullptr) {
      return C18_STATUS_OK;
    }
    running_ = false;
    (void)request_stop();
    if (ctx_ != nullptr) {
      c18_status status = api_.destroy(ctx_);
      if (status != C18_STATUS_OK) {
        return status;
      }
      ctx_ = nullptr;
    }
    if (fail_next_release_) {
      fail_next_release_ = false;
      return C18_STATUS_BUSY;
    }
    unload(handle_);
    handle_ = nullptr;
    return C18_STATUS_OK;
  }

  auto try_lock_probe() -> bool {
    if (!lock_.try_lock()) {
      return false;
    }
    lock_.unlock();
    return true;
  }

private:
  static void C18_CALL host_event(void* userdata, const char* event_name) {
    auto& self = *static_cast<PluginSession*>(userdata);
    std::function<void(std::string_view)> callback;
    {
      std::scoped_lock guard(self.lock_);
      callback = self.callback_;
    }
    if (callback) {
      callback(event_name == nullptr ? std::string_view{} : std::string_view{event_name});
    }
  }

  static auto load(const std::filesystem::path& path) -> void* {
    auto absolute = std::filesystem::absolute(path);
#ifdef _WIN32
    return LoadLibraryExW(absolute.wstring().c_str(), nullptr, LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
#else
    return dlopen(absolute.string().c_str(), RTLD_NOW | RTLD_LOCAL);
#endif
  }

  static auto sym(void* handle, const char* name) -> void* {
#ifdef _WIN32
    return reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(handle), name));
#else
    return dlsym(handle, name);
#endif
  }

  static auto unload(void* handle) -> void {
    if (handle == nullptr) {
      return;
    }
#ifdef _WIN32
    FreeLibrary(static_cast<HMODULE>(handle));
#else
    dlclose(handle);
#endif
  }

  std::mutex lock_;
  c18_api api_{};
  c18_context* ctx_{};
  void* handle_{};
  bool running_{};
  bool fail_next_release_{};
  std::function<void(std::string_view)> callback_;
  std::function<void(std::string_view)> observer_;
};

}  // namespace c18_l04
