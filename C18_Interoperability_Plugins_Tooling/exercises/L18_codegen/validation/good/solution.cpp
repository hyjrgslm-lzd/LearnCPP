#include "tooling_support.hpp"

#include "clang/AST/RecursiveASTVisitor.h"

namespace c18_tooling {
namespace {

class ApiShapeVisitor : public clang::RecursiveASTVisitor<ApiShapeVisitor> {
public:
  explicit ApiShapeVisitor(Manifest& manifest) : manifest_(manifest) {}

  bool VisitFunctionDecl(clang::FunctionDecl* decl) {
    if (decl->getNameAsString() == "c18_get_api") manifest_.exports.insert("c18_get_api");
    return true;
  }

  bool VisitFieldDecl(clang::FieldDecl* field) {
    const auto* record = clang::dyn_cast<clang::RecordDecl>(field->getDeclContext());
    if (record != nullptr && record->getName() == "c18_api") manifest_.fields.push_back(field->getNameAsString());
    return true;
  }

private:
  Manifest& manifest_;
};

}  // namespace

void inspect_manifest(clang::ASTContext& ast, Manifest& manifest) {
  ApiShapeVisitor visitor(manifest);
  visitor.TraverseDecl(ast.getTranslationUnitDecl());
}

auto emit_contract(const Manifest& manifest) -> GeneratedContract {
  bool has_process = false;
  for (const auto& field : manifest.fields) has_process = has_process || field == "process";
  if (!manifest.exports.contains("c18_get_api") || !has_process) return {};
  GeneratedContract generated;
  generated.manifest = "c18_get_api\nc18_api\nprocess\n";
  generated.c_consumer =
      "#include \"c18/abi.h\"\n"
      "_Static_assert(C18_ABI_VERSION == 1u, \"version\");\n"
      "_Static_assert(sizeof(((c18_api*)0)->process) == sizeof(c18_process_fn), \"process slot\");\n";
  generated.cpp_consumer =
      "#include \"c18/abi.h\"\n"
      "static_assert(C18_ABI_VERSION == 1u);\n"
      "static_assert(sizeof(((c18_api*)0)->process) == sizeof(c18_process_fn));\n";
  return generated;
}

}  // namespace c18_tooling
