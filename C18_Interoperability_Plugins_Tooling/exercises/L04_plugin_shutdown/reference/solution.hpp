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
  PluginSession() : control_(std::make_unique<Control>()) {}
  PluginSession(const PluginSession&) = delete;
  auto operator=(const PluginSession&) -> PluginSession& = delete;

  ~PluginSession() {
    if (control_ != nullptr && close() != C18_STATUS_OK) {
      (void)control_.release();
    }
  }

  auto open(const std::filesystem::path& path) -> c18_status {
    {
      std::scoped_lock lock(control_->mutex);
      if (control_->state != State::empty) {
        return C18_STATUS_BUSY;
      }
      control_->state = State::loading;
    }

    void* handle = open_library(path);
    if (handle == nullptr) {
      reset_empty();
      return C18_STATUS_PLUGIN_ERROR;
    }

    auto* symbol = find_symbol(handle, "c18_get_api");
    if (symbol == nullptr) {
      release_library(handle);
      reset_empty();
      return C18_STATUS_MISSING_SYMBOL;
    }

    auto* get_api = reinterpret_cast<c18_get_api_fn>(symbol);
    c18_api api{};
    c18_context* ctx = nullptr;
    try {
      c18_status status = get_api(C18_ABI_VERSION, sizeof(api), &api);
      if (status != C18_STATUS_OK || api.version != C18_ABI_VERSION || api.struct_size < sizeof(c18_api) ||
          api.create == nullptr || api.process == nullptr || api.request_stop == nullptr ||
          api.destroy == nullptr) {
        release_library(handle);
        reset_empty();
        return status == C18_STATUS_OK ? static_cast<c18_status>(C18_STATUS_UNSUPPORTED_VERSION) : status;
      }

      c18_host_api host{C18_ABI_VERSION, sizeof(c18_host_api), control_.get(), &PluginSession::host_event};
      status = api.create(&host, &ctx);
      if (status != C18_STATUS_OK || ctx == nullptr) {
        release_library(handle);
        reset_empty();
        return status == C18_STATUS_OK ? static_cast<c18_status>(C18_STATUS_PLUGIN_ERROR) : status;
      }
    } catch (...) {
      release_library(handle);
      reset_empty();
      return C18_STATUS_PLUGIN_ERROR;
    }

    {
      std::scoped_lock lock(control_->mutex);
      control_->api = api;
      control_->ctx = ctx;
      control_->handle = handle;
      control_->state = State::running;
    }
    return C18_STATUS_OK;
  }

  auto set_callback(std::function<void(std::string_view)> callback) -> void {
    std::scoped_lock lock(control_->mutex);
    control_->callback = std::move(callback);
  }

  auto set_observer(std::function<void(std::string_view)> observer) -> void {
    std::scoped_lock lock(control_->mutex);
    control_->observer = std::move(observer);
  }

  auto fail_next_release_for_test() -> void {
    std::scoped_lock lock(control_->mutex);
    control_->fail_next_release = true;
  }

  auto process(const std::vector<uint8_t>& input, size_t capacity) -> ProcessResult {
    c18_process_fn process_fn = nullptr;
    c18_context* ctx = nullptr;
    {
      std::scoped_lock lock(control_->mutex);
      if (control_->state != State::running) {
        return {C18_STATUS_CLOSING, {}, input.size()};
      }
      ++control_->active_calls;
      process_fn = control_->api.process;
      ctx = control_->ctx;
    }

    ProcessResult result;
    result.bytes.assign(capacity, 0xCCu);
    size_t written = 0;
    try {
      result.status = process_fn(ctx, input.data(), input.size(), result.bytes.data(), result.bytes.size(), &written);
    } catch (...) {
      result.status = C18_STATUS_PLUGIN_ERROR;
    }
    result.required = written;
    if (result.status == C18_STATUS_OK) {
      result.bytes.resize(written);
    }

    finish_call();
    return result;
  }

  auto request_stop() -> c18_status {
    c18_request_stop_fn stop_fn = nullptr;
    c18_context* ctx = nullptr;
    {
      std::scoped_lock lock(control_->mutex);
      if (control_->ctx == nullptr ||
          (control_->state != State::running && control_->state != State::closing)) {
        return C18_STATUS_CLOSING;
      }
      control_->state = State::closing;
      ++control_->active_calls;
      stop_fn = control_->api.request_stop;
      ctx = control_->ctx;
    }

    c18_status status = C18_STATUS_OK;
    try {
      status = stop_fn(ctx);
    } catch (...) {
      status = C18_STATUS_PLUGIN_ERROR;
    }
    finish_call();
    return status;
  }

  auto close() -> c18_status {
    {
      std::scoped_lock lock(control_->mutex);
      if (control_->state == State::running) {
        control_->state = State::closing;
      }
    }
    (void)request_stop();

    c18_status destroy_status = destroy_context_if_ready();
    if (destroy_status != C18_STATUS_OK) {
      return destroy_status;
    }
    return release_handle_if_ready();
  }

  auto try_lock_probe() -> bool {
    if (!control_->mutex.try_lock()) {
      return false;
    }
    control_->mutex.unlock();
    return true;
  }

private:
  enum class State { empty, loading, running, closing, destroying, context_destroyed };

  struct Control {
    std::mutex mutex;
    std::condition_variable drained;
    State state{State::empty};
    c18_api api{};
    c18_context* ctx{};
    void* handle{};
    int active_calls{};
    int callbacks{};
    int borrows{};
    bool fail_next_release{};
    std::function<void(std::string_view)> callback;
    std::function<void(std::string_view)> observer;
  };

  auto reset_empty() -> void {
    std::scoped_lock lock(control_->mutex);
    control_->state = State::empty;
  }

  auto finish_call() -> void {
    {
      std::scoped_lock lock(control_->mutex);
      --control_->active_calls;
    }
    control_->drained.notify_all();
  }

  auto destroy_context_if_ready() -> c18_status {
    c18_destroy_fn destroy_fn = nullptr;
    c18_context* ctx = nullptr;
    std::function<void(std::string_view)> observer;
    {
      std::unique_lock lock(control_->mutex);
      if (control_->state == State::empty || control_->state == State::context_destroyed) {
        return C18_STATUS_OK;
      }
      if (control_->state == State::loading || control_->state == State::destroying) {
        return C18_STATUS_BUSY;
      }
      control_->state = State::closing;
      observer = control_->observer;
      lock.unlock();
      if (observer) {
        observer("close:waiting");
      }
      lock.lock();
      control_->drained.wait(lock, [&] {
        return control_->active_calls == 0 && control_->callbacks == 0 && control_->borrows == 0;
      });
      control_->state = State::destroying;
      ++control_->active_calls;
      destroy_fn = control_->api.destroy;
      ctx = control_->ctx;
    }

    c18_status status = C18_STATUS_OK;
    try {
      status = destroy_fn(ctx);
    } catch (...) {
      status = C18_STATUS_PLUGIN_ERROR;
    }

    {
      std::scoped_lock lock(control_->mutex);
      --control_->active_calls;
      if (status == C18_STATUS_OK) {
        control_->ctx = nullptr;
        control_->state = State::context_destroyed;
      } else {
        control_->state = State::closing;
      }
    }
    control_->drained.notify_all();
    return status;
  }

  auto release_handle_if_ready() -> c18_status {
    void* handle = nullptr;
    bool fail_release = false;
    {
      std::scoped_lock lock(control_->mutex);
      if (control_->state == State::empty) {
        return C18_STATUS_OK;
      }
      if (control_->state != State::context_destroyed || control_->handle == nullptr) {
        return C18_STATUS_BUSY;
      }
      handle = control_->handle;
      fail_release = std::exchange(control_->fail_next_release, false);
    }

    if (fail_release || !release_library(handle)) {
      return C18_STATUS_BUSY;
    }

    {
      std::scoped_lock lock(control_->mutex);
      control_->handle = nullptr;
      control_->api = {};
      control_->state = State::empty;
    }
    return C18_STATUS_OK;
  }

  static void C18_CALL host_event(void* userdata, const char* event_name) {
    auto& control = *static_cast<Control*>(userdata);
    std::function<void(std::string_view)> callback;
    {
      std::scoped_lock lock(control.mutex);
      ++control.callbacks;
      callback = control.callback;
    }
    if (callback) {
      callback(event_name == nullptr ? std::string_view{} : std::string_view{event_name});
    }
    {
      std::scoped_lock lock(control.mutex);
      --control.callbacks;
    }
    control.drained.notify_all();
  }

  static auto open_library(const std::filesystem::path& path) -> void* {
    auto absolute = std::filesystem::absolute(path);
#ifdef _WIN32
    return LoadLibraryExW(absolute.wstring().c_str(), nullptr, LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
#else
    return dlopen(absolute.string().c_str(), RTLD_NOW | RTLD_LOCAL);
#endif
  }

  static auto find_symbol(void* handle, const char* name) -> void* {
#ifdef _WIN32
    return reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(handle), name));
#else
    return dlsym(handle, name);
#endif
  }

  static auto release_library(void* handle) -> bool {
    if (handle == nullptr) {
      return true;
    }
#ifdef _WIN32
    return FreeLibrary(static_cast<HMODULE>(handle)) != 0;
#else
    return dlclose(handle) == 0;
#endif
  }

  std::unique_ptr<Control> control_;
};

}  // namespace c18_l04
