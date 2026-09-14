#include "tooling_support.hpp"

namespace c18_tooling {

void inspect_manifest(clang::ASTContext&, Manifest&) {
  // TODO: collect c18_get_api and c18_api fields from the parsed ABI header.
}

auto emit_contract(const Manifest&) -> GeneratedContract {
  return {};
}

}  // namespace c18_tooling
