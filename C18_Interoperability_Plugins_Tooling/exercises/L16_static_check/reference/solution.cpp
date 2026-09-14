#include "tooling_support.hpp"

#include "clang/AST/RecursiveASTVisitor.h"

namespace c18_tooling {
namespace {

class AbiVisitor : public clang::RecursiveASTVisitor<AbiVisitor> {
public:
  AbiVisitor(clang::ASTContext& ast, std::vector<Finding>& findings) : ast_(ast), findings_(findings) {}

  bool shouldVisitTemplateInstantiations() const { return true; }

  bool VisitFunctionDecl(clang::FunctionDecl* decl) {
    if (!decl->isExternC() || !decl->getName().starts_with("c18_")) return true;
    check_type(decl->getReturnType(), decl->getLocation(), "return type", decl->getNameAsString());
    for (clang::ParmVarDecl* param : decl->parameters()) {
      check_type(param->getType(), param->getLocation(), "parameter type", decl->getNameAsString());
    }
    return true;
  }

  bool VisitFieldDecl(clang::FieldDecl* field) {
    const auto* parent = clang::dyn_cast<clang::RecordDecl>(field->getDeclContext());
    if (parent == nullptr || parent->getName() != "c18_api") return true;
    check_type(field->getType(), field->getLocation(), "function-table field", field->getNameAsString());
    return true;
  }

private:
  void check_type(clang::QualType type, clang::SourceLocation loc, llvm::StringRef role, llvm::StringRef owner) {
    std::string printed = type_text(type);
    if (printed.find("std::") == std::string::npos && printed.find("basic_string") == std::string::npos &&
        is_plain_abi_type(type)) {
      return;
    }
    auto [file, line] = location_text(ast_.getSourceManager(), loc);
    findings_.push_back({file, line, "dangerous exported ABI type in " + role.str() + " of " + owner.str() + ": " + printed});
  }

  clang::ASTContext& ast_;
  std::vector<Finding>& findings_;
};

}  // namespace

void inspect_abi(clang::ASTContext& ast, std::vector<Finding>& findings) {
  AbiVisitor visitor(ast, findings);
  visitor.TraverseDecl(ast.getTranslationUnitDecl());
}

}  // namespace c18_tooling
