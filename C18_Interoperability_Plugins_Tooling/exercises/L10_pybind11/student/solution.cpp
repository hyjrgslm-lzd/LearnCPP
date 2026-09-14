#include <pybind11/pybind11.h>

#include <memory>
#include <string>
#include <utility>

namespace py = pybind11;

namespace {
int owner_destructed = 0;
int token_destructed = 0;

class Owner {
public:
  explicit Owner(std::string bytes) : bytes_(std::move(bytes)) {}
  ~Owner() { ++owner_destructed; }
  auto view() const -> py::bytes { return py::bytes(bytes_); }

private:
  std::string bytes_;
};

class OwnerAlias {
public:
  explicit OwnerAlias(std::shared_ptr<Owner> owner) : owner_(std::move(owner)) {}
  auto view_bytes() const -> py::bytes { return owner_->view(); }

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
}  // namespace

PYBIND11_MODULE(c18_l10_student, m) {
  py::class_<Owner, std::shared_ptr<Owner>>(m, "Owner")
      .def(py::init<std::string>())
      .def("view", &Owner::view);
  py::class_<OwnerAlias>(m, "OwnerAlias").def("view_bytes", &OwnerAlias::view_bytes);
  py::class_<Token, std::shared_ptr<Token>>(m, "Token")
      .def(py::init<std::string>())
      .def_property_readonly("name", &Token::name);
  py::class_<Registry>(m, "Registry")
      .def(py::init<>())
      .def("remember", &Registry::remember)
      .def_property_readonly("last", &Registry::last);
  py::class_<Transformer, std::shared_ptr<Transformer>>(m, "Transformer")
      .def(py::init<>())
      .def("transform", &Transformer::transform);

  m.def("make_owner_alias", [](const std::shared_ptr<Owner>& owner) { return OwnerAlias(owner); });
  m.def("owner_destructed", [] { return owner_destructed; });
  m.def("token_destructed", [] { return token_destructed; });
  m.def("reset_counts", [] {
    owner_destructed = 0;
    token_destructed = 0;
  });
  m.def("trace", [] { return py::list(); });
  m.def("transform", [](py::bytes payload) { return payload; });
  m.def("raise_cpp", [] {});
  m.def("call_transformer_from_worker", [](const std::shared_ptr<Transformer>& transformer, py::bytes payload) {
    std::string input = payload;
    return py::bytes(transformer->transform(input));
  });
}
