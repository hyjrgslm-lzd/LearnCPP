#include "tooling_support.hpp"

#include "clang/ASTMatchers/ASTMatchers.h"

#include <memory>
#include <set>
#include <tuple>
#include <utility>
#include <vector>

namespace c18_tooling {
namespace {
using namespace clang::ast_matchers;

class BlindCallback : public MatchFinder::MatchCallback {
public:
  BlindCallback(clang::tooling::Replacements& replacements, std::string old_symbol, std::string new_symbol)
      : replacements_(replacements), old_symbol_(std::move(old_symbol)), new_symbol_(std::move(new_symbol)) {}

  void run(const MatchFinder::MatchResult& result) override {
    const auto* ref = result.Nodes.getNodeAs<clang::DeclRefExpr>("ref");
    const auto* decl = result.Nodes.getNodeAs<clang::NamedDecl>("decl");
    if (ref == nullptr && decl == nullptr) return;
    const auto& sm = *result.SourceManager;
    clang::SourceLocation loc = ref != nullptr ? ref->getLocation() : decl->getLocation();
    std::string ignored;
    add_replacement(replacements_, seen_, clang::tooling::Replacement(sm, loc, old_symbol_.size(), new_symbol_), ignored);
  }

private:
  clang::tooling::Replacements& replacements_;
  std::string old_symbol_;
  std::string new_symbol_;
  std::set<std::tuple<std::string, unsigned, unsigned, std::string>> seen_;
};

}  // namespace

void register_rewrite_matchers(MatchFinder& finder,
                               clang::tooling::Replacements& replacements,
                               std::vector<Finding>&,
                               std::string old_symbol,
                               std::string new_symbol) {
  static std::vector<std::unique_ptr<BlindCallback>> callbacks;
  callbacks.push_back(std::make_unique<BlindCallback>(replacements, old_symbol, new_symbol));
  auto& callback = *callbacks.back();
  finder.addMatcher(namedDecl(hasName(old_symbol)).bind("decl"), &callback);
  finder.addMatcher(declRefExpr(to(namedDecl(hasName(old_symbol)))).bind("ref"), &callback);
}

}  // namespace c18_tooling
