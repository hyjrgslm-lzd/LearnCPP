#include <pybind11/embed.h>

#include <iostream>
#include <string>

namespace py = pybind11;

int main() {
  py::scoped_interpreter guard;
  {
    py::dict locals;
    locals["payload"] = py::bytes(std::string("azA\0\xffm", 6));
    py::exec("result = payload.upper()", py::globals(), locals);
    const std::string result = locals["result"].cast<std::string>();
    if (result != std::string("AZA\0\xffM", 6)) {
      std::cout << "check failed: scoped_interpreter byte transform changed semantics\n";
      return 1;
    }
  }
  std::cout << "L10 pybind11 embedding PASS\n";
  return 0;
}
