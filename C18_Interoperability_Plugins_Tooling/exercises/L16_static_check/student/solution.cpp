#include "tooling_support.hpp"

namespace c18_tooling {

void inspect_abi(clang::ASTContext&, std::vector<Finding>&) {
  // TODO: visit extern "C" c18_* declarations and c18_api function table fields.
}

}  // namespace c18_tooling
