#include "tooling_support.hpp"

#include "clang/AST/RecursiveASTVisitor.h"

namespace c18_tooling {
namespace {

class DefinitionOnlyVisitor : public clang::RecursiveASTVisitor<DefinitionOnlyVisitor> {
public:
  DefinitionOnlyVisitor(clang::ASTContext& ast, std::vector<Finding>& findings) : ast_(ast), findings_(findings) {}

  bool VisitFunctionDecl(clang::FunctionDecl* decl) {
    if (!decl->isExternC() || !decl->isThisDeclarationADefinition() || !decl->getName().starts_with("c18_")) return true;
    std::string spelling = type_text(decl->getReturnType());
    if (spelling.find("std::") != std::string::npos) {
      auto [file, line] = location_text(ast_.getSourceManager(), decl->getLocation());
      findings_.push_back({file, line, "dangerous exported ABI type: " + spelling});
    }
    return true;
  }

private:
  clang::ASTContext& ast_;
  std::vector<Finding>& findings_;
};

}  // namespace

void inspect_abi(clang::ASTContext& ast, std::vector<Finding>& findings) {
  DefinitionOnlyVisitor visitor(ast, findings);
  visitor.TraverseDecl(ast.getTranslationUnitDecl());
}

}  // namespace c18_tooling
