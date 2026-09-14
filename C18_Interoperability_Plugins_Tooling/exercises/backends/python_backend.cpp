#include "python_backend.hpp"

#include <Python.h>

#include <mutex>
#include <stdexcept>
#include <string>

namespace {

class PyObjectRef {
public:
  explicit PyObjectRef(PyObject* value = nullptr) : value_(value) {}
  PyObjectRef(const PyObjectRef&) = delete;
  auto operator=(const PyObjectRef&) -> PyObjectRef& = delete;
  PyObjectRef(PyObjectRef&& other) noexcept : value_(other.value_) { other.value_ = nullptr; }
  ~PyObjectRef() { Py_XDECREF(value_); }
  auto get() const -> PyObject* { return value_; }
  auto release() -> PyObject* {
    PyObject* value = value_;
    value_ = nullptr;
    return value;
  }

private:
  PyObject* value_{};
};

[[noreturn]] void throw_python_error(const char* prefix) {
  PyObject *type = nullptr, *value = nullptr, *traceback = nullptr;
  PyErr_Fetch(&type, &value, &traceback);
  PyObjectRef type_ref(type), value_ref(value), traceback_ref(traceback);
  PyObjectRef text(value ? PyObject_Str(value) : nullptr);
  std::string message = prefix;
  if (text.get()) {
    if (const char* utf8 = PyUnicode_AsUTF8(text.get())) {
      message += ": ";
      message += utf8;
    }
  }
  throw std::runtime_error(message);
}

}  // namespace

namespace c18 {

std::vector<std::uint8_t> python_transform(std::span<const std::uint8_t> input) {
  static std::mutex python_mutex;
  std::scoped_lock lock(python_mutex);

  PyStatus status;
  PyConfig config;
  PyConfig_InitIsolatedConfig(&config);
  config.site_import = 0;
  status = PyConfig_SetBytesString(&config, &config.home, C18_PYTHON_HOME);
  if (PyStatus_Exception(status)) {
    PyConfig_Clear(&config);
    throw std::runtime_error("PyConfig_SetBytesString failed");
  }
  status = Py_InitializeFromConfig(&config);
  PyConfig_Clear(&config);
  if (PyStatus_Exception(status)) {
    throw std::runtime_error("Py_InitializeFromConfig failed");
  }

  std::vector<std::uint8_t> output;
  try {
    PyObjectRef globals(PyDict_New());
    if (!globals.get()) {
      throw_python_error("create globals");
    }
    PyObjectRef builtins(PyEval_GetBuiltins());
    Py_INCREF(builtins.get());
    if (PyDict_SetItemString(globals.get(), "__builtins__", builtins.get()) < 0) {
      throw_python_error("set builtins");
    }
    const char* code =
        "def transform(payload):\n"
        "    return bytes((b - 32 if 97 <= b <= 122 else b) for b in payload)\n";
    PyObjectRef compiled(Py_CompileString(code, "<c18-python-backend>", Py_file_input));
    if (!compiled.get()) {
      throw_python_error("compile transform");
    }
    PyObjectRef ignored(PyEval_EvalCode(compiled.get(), globals.get(), globals.get()));
    if (!ignored.get()) {
      throw_python_error("define transform");
    }
    PyObject* borrowed_func = PyDict_GetItemString(globals.get(), "transform");
    if (!borrowed_func) {
      throw std::runtime_error("transform function missing");
    }
    PyObjectRef func(Py_NewRef(borrowed_func));
    PyObjectRef arg(PyBytes_FromStringAndSize(reinterpret_cast<const char*>(input.data()),
                                             static_cast<Py_ssize_t>(input.size())));
    if (!arg.get()) {
      throw_python_error("build input bytes");
    }
    PyObjectRef result(PyObject_CallFunctionObjArgs(func.get(), arg.get(), nullptr));
    if (!result.get()) {
      throw_python_error("call transform");
    }
    char* bytes = nullptr;
    Py_ssize_t size = 0;
    if (PyBytes_AsStringAndSize(result.get(), &bytes, &size) < 0) {
      throw_python_error("read result bytes");
    }
    output.assign(reinterpret_cast<std::uint8_t*>(bytes),
                  reinterpret_cast<std::uint8_t*>(bytes) + size);
  } catch (...) {
    Py_FinalizeEx();
    throw;
  }

  if (Py_FinalizeEx() < 0) {
    throw std::runtime_error("Py_FinalizeEx failed");
  }
  return output;
}

}  // namespace c18
