#include "tooling_support.hpp"

namespace c18_tooling {

void inspect_manifest(clang::ASTContext&, Manifest& manifest) {
  manifest.exports.insert("python_binding");
}

auto emit_contract(const Manifest&) -> GeneratedContract {
  return {"binding.py\n", "", ""};
}

}  // namespace c18_tooling
