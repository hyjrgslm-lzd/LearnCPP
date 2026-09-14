#include "tooling_support.hpp"

namespace c18_tooling {

void register_rewrite_matchers(clang::ast_matchers::MatchFinder&,
                               clang::tooling::Replacements&,
                               std::vector<Finding>&,
                               std::string,
                               std::string) {
  // TODO: register declaration/reference matchers and reject unsafe source locations.
}

}  // namespace c18_tooling
