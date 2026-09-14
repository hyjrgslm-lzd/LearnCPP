#include "tooling_support.hpp"

#include "clang/AST/RecursiveASTVisitor.h"

namespace c18_tooling {
namespace {

class ExportVisitor : public clang::RecursiveASTVisitor<ExportVisitor> {
public:
  ExportVisitor(clang::ASTContext& ast, std::vector<Finding>& findings) : ast_(ast), findings_(findings) {}

  bool VisitFunctionDecl(clang::FunctionDecl* decl) {
    if (!decl->isExternC() || !decl->getIdentifier() || !decl->getName().starts_with("c18_")) return true;
    inspect(decl->getReturnType(), decl->getLocation(), decl->getNameAsString());
    for (const clang::ParmVarDecl* param : decl->parameters()) inspect(param->getType(), param->getLocation(), decl->getNameAsString());
    return true;
  }

  bool VisitFieldDecl(clang::FieldDecl* field) {
    const auto* record = clang::dyn_cast<clang::RecordDecl>(field->getDeclContext());
    if (record == nullptr || record->getName() != "c18_api") return true;
    inspect(field->getType(), field->getLocation(), "c18_api." + field->getNameAsString());
    return true;
  }

private:
  void inspect(clang::QualType type, clang::SourceLocation loc, const std::string& owner) {
    std::string spelling = type_text(type);
    if (spelling.find("std::") == std::string::npos && spelling.find("&") == std::string::npos &&
        spelling.find("vector<") == std::string::npos && is_plain_abi_type(type)) {
      return;
    }
    auto [file, line] = location_text(ast_.getSourceManager(), loc);
    findings_.push_back({file, line, "dangerous exported ABI type in " + owner + ": " + spelling});
  }

  clang::ASTContext& ast_;
  std::vector<Finding>& findings_;
};

}  // namespace

void inspect_abi(clang::ASTContext& ast, std::vector<Finding>& findings) {
  ExportVisitor visitor(ast, findings);
  visitor.TraverseDecl(ast.getTranslationUnitDecl());
}

}  // namespace c18_tooling
