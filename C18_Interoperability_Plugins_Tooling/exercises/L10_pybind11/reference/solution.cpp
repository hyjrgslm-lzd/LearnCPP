#include <pybind11/functional.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <exception>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace py = pybind11;

namespace {
int owner_destructed = 0;
int token_destructed = 0;
std::vector<std::string> trace;

void add_trace(std::string item) { trace.push_back(std::move(item)); }

class View {
public:
  explicit View(std::string bytes) : bytes_(std::move(bytes)) {}
  auto bytes() const -> py::bytes { return py::bytes(bytes_); }

private:
  std::string bytes_;
};

class Owner {
public:
  explicit Owner(std::string bytes) : view_(std::move(bytes)) {}
  ~Owner() { ++owner_destructed; }
  auto view() -> View& { return view_; }

private:
  View view_;
};

class OwnerAlias {
public:
  explicit OwnerAlias(std::shared_ptr<Owner> owner) : owner_(std::move(owner)) {}
  auto view_bytes() const -> py::bytes { return owner_->view().bytes(); }

private:
  std::shared_ptr<Owner> owner_;
};

class Token {
public:
  explicit Token(std::string name) : name_(std::move(name)) {}
  ~Token() { ++token_destructed; }
  auto name() const -> const std::string& { return name_; }

private:
  std::string name_;
};

class Registry {
public:
  void remember(const std::shared_ptr<Token>& token) { last_ = token->name(); }
  auto last() const -> const std::string& { return last_; }

private:
  std::string last_;
};

class Transformer {
public:
  virtual ~Transformer() = default;
  virtual auto transform(std::string payload) -> std::string { return "base:" + payload; }
};

class PyTransformer : public Transformer {
public:
  using Transformer::Transformer;
  auto transform(std::string payload) -> std::string override {
    PYBIND11_OVERRIDE(std::string, Transformer, transform, std::move(payload));
  }
};

auto upper_ascii(std::string input) -> std::string {
  for (char& c : input) {
    if (c >= 'a' && c <= 'z') {
      c = static_cast<char>(c - 32);
    }
  }
  return input;
}
}  // namespace

PYBIND11_MODULE(c18_l10_reference, m) {
  py::class_<View>(m, "View").def("__bytes__", &View::bytes);
  py::class_<Owner, std::shared_ptr<Owner>>(m, "Owner")
      .def(py::init<std::string>())
      .def("view", &Owner::view, py::return_value_policy::reference_internal);
  py::class_<OwnerAlias>(m, "OwnerAlias").def("view_bytes", &OwnerAlias::view_bytes);
  py::class_<Token, std::shared_ptr<Token>>(m, "Token")
      .def(py::init<std::string>())
      .def_property_readonly("name", &Token::name);
  py::class_<Registry>(m, "Registry")
      .def(py::init<>())
      .def("remember", &Registry::remember, py::keep_alive<1, 2>())
      .def_property_readonly("last", &Registry::last);
  py::class_<Transformer, PyTransformer, std::shared_ptr<Transformer>>(m, "Transformer")
      .def(py::init<>())
      .def("transform", &Transformer::transform);

  m.def("make_owner_alias", [](const std::shared_ptr<Owner>& owner) { return OwnerAlias(owner); });
  m.def("owner_destructed", [] { return owner_destructed; });
  m.def("token_destructed", [] { return token_destructed; });
  m.def("reset_counts", [] {
    owner_destructed = 0;
    token_destructed = 0;
    trace.clear();
  });
  m.def("trace", [] { return trace; });
  m.def("transform", [](py::bytes payload) { return py::bytes(upper_ascii(payload)); });
  m.def("raise_cpp", [] { throw std::runtime_error("mapped to Python exception"); });
  m.def("call_transformer_from_worker", [](const std::shared_ptr<Transformer>& transformer, py::bytes payload) {
    std::string input = payload;
    std::string output;
    std::exception_ptr error;
    std::thread worker([&] {
      try {
        py::gil_scoped_acquire gil;
        add_trace("worker_gil");
        output = transformer->transform(input);
        add_trace("override_returned:" + output);
      } catch (...) {
        error = std::current_exception();
      }
    });
    {
      py::gil_scoped_release release;
      worker.join();
    }
    if (error) {
      std::rethrow_exception(error);
    }
    return py::bytes(output);
  });
}
