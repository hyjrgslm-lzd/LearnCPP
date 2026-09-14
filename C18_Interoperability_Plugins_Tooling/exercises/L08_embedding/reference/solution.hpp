#pragma once

#include <Python.h>

#include <condition_variable>
#include <cstdint>
#include <exception>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace c18_l08 {

enum class EventKind {
  main_released_gil,
  worker_started,
  worker_gil_acquired,
  python_result,
  py_refs_released,
  stop_observed,
  worker_joined,
  early_finalize_rejected,
  finalized
};

struct Event {
  EventKind kind{};
  std::thread::id thread{};
  std::string detail;
};

class EmbeddedPython {
public:
  EmbeddedPython() {
    PyConfig config;
    PyConfig_InitIsolatedConfig(&config);
    config.site_import = 0;
    PyStatus status = PyConfig_SetBytesString(&config, &config.home, C18_PYTHON_HOME);
    if (PyStatus_Exception(status)) {
      PyConfig_Clear(&config);
      throw std::runtime_error("python home failed");
    }
    status = Py_InitializeFromConfig(&config);
    PyConfig_Clear(&config);
    if (PyStatus_Exception(status)) {
      throw std::runtime_error("python init failed");
    }
    main_state_ = PyEval_SaveThread();
    record(EventKind::main_released_gil);
    worker_ = std::thread(&EmbeddedPython::run, this);
  }

  EmbeddedPython(const EmbeddedPython&) = delete;
  auto operator=(const EmbeddedPython&) -> EmbeddedPython& = delete;

  ~EmbeddedPython() {
    try {
      stop();
      join();
    } catch (...) {
    }
  }

  auto transform(const std::vector<std::uint8_t>& input) -> std::vector<std::uint8_t> {
    Request request;
    request.input = input;
    {
      std::lock_guard lock(mutex_);
      if (stop_requested_) {
        throw std::runtime_error("worker is closing");
      }
      pending_ = &request;
    }
    cv_.notify_all();
    std::unique_lock lock(mutex_);
    cv_.wait(lock, [&] { return request.done; });
    if (request.error) {
      std::rethrow_exception(request.error);
    }
    return std::move(*request.output);
  }

  void stop() {
    {
      std::lock_guard lock(mutex_);
      stop_requested_ = true;
    }
    cv_.notify_all();
  }

  void join() {
    if (worker_.joinable()) {
      worker_.join();
      record(EventKind::worker_joined);
    }
    finalize_after_join();
  }

  [[nodiscard]] auto released_before_shutdown() const -> bool {
    const auto snapshot = events();
    auto release_pos = snapshot.size();
    auto finalize_pos = snapshot.size();
    for (std::size_t i = 0; i < snapshot.size(); ++i) {
      if (snapshot[i].kind == EventKind::py_refs_released && release_pos == snapshot.size()) {
        release_pos = i;
      }
      if (snapshot[i].kind == EventKind::finalized && finalize_pos == snapshot.size()) {
        finalize_pos = i;
      }
    }
    return release_pos < finalize_pos;
  }

  [[nodiscard]] auto events() const -> std::vector<Event> {
    std::lock_guard lock(events_mutex_);
    return events_;
  }

protected:
  auto finalize_after_join() -> bool {
    if (finalized_) {
      return true;
    }
    if (worker_.joinable()) {
      record(EventKind::early_finalize_rejected);
      return false;
    }
    if (Py_IsInitialized()) {
      PyEval_RestoreThread(main_state_);
      main_state_ = nullptr;
      if (Py_FinalizeEx() < 0) {
        throw std::runtime_error("python finalize failed");
      }
    }
    finalized_ = true;
    record(EventKind::finalized);
    return true;
  }

private:
  struct Request {
    std::vector<std::uint8_t> input;
    std::optional<std::vector<std::uint8_t>> output;
    std::exception_ptr error;
    bool done{};
  };

  void run() {
    record(EventKind::worker_started);
    for (;;) {
      Request* request = nullptr;
      {
        std::unique_lock lock(mutex_);
        cv_.wait(lock, [&] { return pending_ != nullptr || stop_requested_; });
        request = pending_;
        pending_ = nullptr;
        if (!request && stop_requested_) {
          record(EventKind::stop_observed);
          return;
        }
      }
      try {
        request->output = run_python(request->input);
      } catch (...) {
        request->error = std::current_exception();
      }
      {
        std::lock_guard lock(mutex_);
        request->done = true;
      }
      cv_.notify_all();
    }
  }

  auto run_python(const std::vector<std::uint8_t>& input) -> std::vector<std::uint8_t> {
    PyGILState_STATE gil = PyGILState_Ensure();
    record(EventKind::worker_gil_acquired);
    PyObject* payload = PyBytes_FromStringAndSize(reinterpret_cast<const char*>(input.data()),
                                                  static_cast<Py_ssize_t>(input.size()));
    PyObject* globals = PyDict_New();
    PyObject* result = nullptr;
    if (payload && globals) {
      PyDict_SetItemString(globals, "__builtins__", PyEval_GetBuiltins());
      PyDict_SetItemString(globals, "payload", payload);
      result = PyRun_String("bytes((b - 32 if 97 <= b <= 122 else b) for b in payload)",
                            Py_eval_input, globals, globals);
    }
    char* data = nullptr;
    Py_ssize_t size = 0;
    if (!result || PyBytes_AsStringAndSize(result, &data, &size) < 0) {
      Py_XDECREF(result);
      Py_XDECREF(globals);
      Py_XDECREF(payload);
      record(EventKind::py_refs_released);
      PyGILState_Release(gil);
      throw std::runtime_error("python transform failed");
    }
    std::vector<std::uint8_t> out(reinterpret_cast<std::uint8_t*>(data),
                                  reinterpret_cast<std::uint8_t*>(data) + size);
    record(EventKind::python_result, std::string(reinterpret_cast<char*>(out.data()), out.size()));
    Py_DECREF(result);
    Py_DECREF(globals);
    Py_DECREF(payload);
    record(EventKind::py_refs_released);
    PyGILState_Release(gil);
    return out;
  }

  void record(EventKind kind, std::string detail = {}) const {
    std::lock_guard lock(events_mutex_);
    events_.push_back(Event{kind, std::this_thread::get_id(), std::move(detail)});
  }

  mutable std::mutex mutex_;
  std::condition_variable cv_;
  Request* pending_{};
  bool stop_requested_{};
  std::thread worker_;
  PyThreadState* main_state_{};
  bool finalized_{};

  mutable std::mutex events_mutex_;
  mutable std::vector<Event> events_;
};

}  // namespace c18_l08
