#pragma once

#include "c18/abi.h"

#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
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
  PluginSession() : shared_(std::make_unique<Shared>()) {}
  PluginSession(const PluginSession&) = delete;
  auto operator=(const PluginSession&) -> PluginSession& = delete;

  ~PluginSession() {
    if (shared_ != nullptr && close() != C18_STATUS_OK) {
      (void)shared_.release();
    }
  }

  auto open(const std::filesystem::path& path) -> c18_status {
    {
      std::scoped_lock guard(shared_->lock);
      if (shared_->phase != Phase::empty) {
        return C18_STATUS_BUSY;
      }
      shared_->phase = Phase::loading;
    }

    void* library = load_library(path);
    if (library == nullptr) {
      mark_empty();
      return C18_STATUS_PLUGIN_ERROR;
    }

    auto* raw_entry = load_symbol(library, "c18_get_api");
    if (raw_entry == nullptr) {
      unload_library(library);
      mark_empty();
      return C18_STATUS_MISSING_SYMBOL;
    }

    c18_api api{};
    c18_context* ctx = nullptr;
    try {
      auto* entry = reinterpret_cast<c18_get_api_fn>(raw_entry);
      c18_status status = entry(C18_ABI_VERSION, sizeof(api), &api);
      if (status != C18_STATUS_OK || api.version != C18_ABI_VERSION || api.struct_size < sizeof(c18_api) ||
          api.create == nullptr || api.process == nullptr || api.request_stop == nullptr ||
          api.destroy == nullptr) {
        unload_library(library);
        mark_empty();
        return status == C18_STATUS_OK ? static_cast<c18_status>(C18_STATUS_UNSUPPORTED_VERSION) : status;
      }

      c18_host_api host{C18_ABI_VERSION, sizeof(c18_host_api), shared_.get(), &PluginSession::dispatch_event};
      status = api.create(&host, &ctx);
      if (status != C18_STATUS_OK || ctx == nullptr) {
        unload_library(library);
        mark_empty();
        return status == C18_STATUS_OK ? static_cast<c18_status>(C18_STATUS_PLUGIN_ERROR) : status;
      }
    } catch (...) {
      unload_library(library);
      mark_empty();
      return C18_STATUS_PLUGIN_ERROR;
    }

    {
      std::scoped_lock guard(shared_->lock);
      shared_->api = api;
      shared_->ctx = ctx;
      shared_->library = library;
      shared_->phase = Phase::running;
    }
    return C18_STATUS_OK;
  }

  auto set_callback(std::function<void(std::string_view)> callback) -> void {
    std::scoped_lock guard(shared_->lock);
    shared_->callback = std::move(callback);
  }

  auto set_observer(std::function<void(std::string_view)> observer) -> void {
    std::scoped_lock guard(shared_->lock);
    shared_->observer = std::move(observer);
  }

  auto fail_next_release_for_test() -> void {
    std::scoped_lock guard(shared_->lock);
    shared_->release_should_fail = true;
  }

  auto process(const std::vector<uint8_t>& input, size_t capacity) -> ProcessResult {
    c18_process_fn fn = nullptr;
    c18_context* ctx = nullptr;
    if (!enter_process(fn, ctx)) {
      return {C18_STATUS_CLOSING, {}, input.size()};
    }

    ProcessResult result;
    result.bytes.assign(capacity, 0xCCu);
    size_t written = 0;
    try {
      result.status = fn(ctx, input.data(), input.size(), result.bytes.data(), result.bytes.size(), &written);
    } catch (...) {
      result.status = C18_STATUS_PLUGIN_ERROR;
    }
    result.required = written;
    if (result.status == C18_STATUS_OK) {
      result.bytes.resize(written);
    }
    leave_plugin_call();
    return result;
  }

  auto request_stop() -> c18_status {
    c18_request_stop_fn fn = nullptr;
    c18_context* ctx = nullptr;
    {
      std::scoped_lock guard(shared_->lock);
      if (shared_->ctx == nullptr || (shared_->phase != Phase::running && shared_->phase != Phase::closing)) {
        return C18_STATUS_CLOSING;
      }
      shared_->phase = Phase::closing;
      ++shared_->in_plugin;
      fn = shared_->api.request_stop;
      ctx = shared_->ctx;
    }

    c18_status status = C18_STATUS_OK;
    try {
      status = fn(ctx);
    } catch (...) {
      status = C18_STATUS_PLUGIN_ERROR;
    }
    leave_plugin_call();
    return status;
  }

  auto close() -> c18_status {
    {
      std::scoped_lock guard(shared_->lock);
      if (shared_->phase == Phase::running) {
        shared_->phase = Phase::closing;
      }
    }
    (void)request_stop();

    c18_status destroyed = destroy_when_idle();
    if (destroyed != C18_STATUS_OK) {
      return destroyed;
    }
    return release_when_destroyed();
  }

  auto try_lock_probe() -> bool {
    if (!shared_->lock.try_lock()) {
      return false;
    }
    shared_->lock.unlock();
    return true;
  }

private:
  enum class Phase { empty, loading, running, closing, destroying, destroyed };

  struct Shared {
    std::mutex lock;
    std::condition_variable idle;
    Phase phase{Phase::empty};
    c18_api api{};
    c18_context* ctx{};
    void* library{};
    int in_plugin{};
    int in_callback{};
    int borrows{};
    bool release_should_fail{};
    std::function<void(std::string_view)> callback;
    std::function<void(std::string_view)> observer;
  };

  auto mark_empty() -> void {
    std::scoped_lock guard(shared_->lock);
    shared_->phase = Phase::empty;
  }

  auto enter_process(c18_process_fn& fn, c18_context*& ctx) -> bool {
    std::scoped_lock guard(shared_->lock);
    if (shared_->phase != Phase::running) {
      return false;
    }
    ++shared_->in_plugin;
    fn = shared_->api.process;
    ctx = shared_->ctx;
    return true;
  }

  auto leave_plugin_call() -> void {
    {
      std::scoped_lock guard(shared_->lock);
      --shared_->in_plugin;
    }
    shared_->idle.notify_all();
  }

  auto destroy_when_idle() -> c18_status {
    c18_destroy_fn fn = nullptr;
    c18_context* ctx = nullptr;
    std::function<void(std::string_view)> observer;
    {
      std::unique_lock guard(shared_->lock);
      if (shared_->phase == Phase::empty || shared_->phase == Phase::destroyed) {
        return C18_STATUS_OK;
      }
      if (shared_->phase == Phase::loading || shared_->phase == Phase::destroying) {
        return C18_STATUS_BUSY;
      }
      observer = shared_->observer;
      guard.unlock();
      if (observer) {
        observer("close:waiting");
      }
      guard.lock();
      shared_->idle.wait(guard, [&] {
        return shared_->in_plugin == 0 && shared_->in_callback == 0 && shared_->borrows == 0;
      });
      shared_->phase = Phase::destroying;
      ++shared_->in_plugin;
      fn = shared_->api.destroy;
      ctx = shared_->ctx;
    }

    c18_status status = C18_STATUS_OK;
    try {
      status = fn(ctx);
    } catch (...) {
      status = C18_STATUS_PLUGIN_ERROR;
    }

    {
      std::scoped_lock guard(shared_->lock);
      --shared_->in_plugin;
      if (status == C18_STATUS_OK) {
        shared_->ctx = nullptr;
        shared_->phase = Phase::destroyed;
      } else {
        shared_->phase = Phase::closing;
      }
    }
    shared_->idle.notify_all();
    return status;
  }

  auto release_when_destroyed() -> c18_status {
    void* library = nullptr;
    bool fail = false;
    {
      std::scoped_lock guard(shared_->lock);
      if (shared_->phase == Phase::empty) {
        return C18_STATUS_OK;
      }
      if (shared_->phase != Phase::destroyed || shared_->library == nullptr) {
        return C18_STATUS_BUSY;
      }
      library = shared_->library;
      fail = std::exchange(shared_->release_should_fail, false);
    }

    if (fail || !unload_library(library)) {
      return C18_STATUS_BUSY;
    }

    {
      std::scoped_lock guard(shared_->lock);
      shared_->library = nullptr;
      shared_->api = {};
      shared_->phase = Phase::empty;
    }
    return C18_STATUS_OK;
  }

  static void C18_CALL dispatch_event(void* userdata, const char* event_name) {
    auto& shared = *static_cast<Shared*>(userdata);
    std::function<void(std::string_view)> callback;
    {
      std::scoped_lock guard(shared.lock);
      ++shared.in_callback;
      callback = shared.callback;
    }
    if (callback) {
      callback(event_name == nullptr ? std::string_view{} : std::string_view{event_name});
    }
    {
      std::scoped_lock guard(shared.lock);
      --shared.in_callback;
    }
    shared.idle.notify_all();
  }

  static auto load_library(const std::filesystem::path& path) -> void* {
    auto absolute = std::filesystem::absolute(path);
#ifdef _WIN32
    return LoadLibraryExW(absolute.wstring().c_str(), nullptr, LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
#else
    return dlopen(absolute.string().c_str(), RTLD_NOW | RTLD_LOCAL);
#endif
  }

  static auto load_symbol(void* library, const char* name) -> void* {
#ifdef _WIN32
    return reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(library), name));
#else
    return dlsym(library, name);
#endif
  }

  static auto unload_library(void* library) -> bool {
    if (library == nullptr) {
      return true;
    }
#ifdef _WIN32
    return FreeLibrary(static_cast<HMODULE>(library)) != 0;
#else
    return dlclose(library) == 0;
#endif
  }

  std::unique_ptr<Shared> shared_;
};

}  // namespace c18_l04
