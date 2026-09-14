#include "tooling_support.hpp"

#include "clang/AST/RecursiveASTVisitor.h"

namespace c18_tooling {
namespace {

class ManifestVisitor : public clang::RecursiveASTVisitor<ManifestVisitor> {
public:
  explicit ManifestVisitor(Manifest& manifest) : manifest_(manifest) {}

  bool VisitFunctionDecl(clang::FunctionDecl* decl) {
    if (decl->isExternC() && decl->getName() == "c18_get_api") manifest_.exports.insert("c18_get_api");
    return true;
  }

  bool VisitRecordDecl(clang::RecordDecl* decl) {
    if (decl->getName() != "c18_api" || !decl->isCompleteDefinition()) return true;
    for (const clang::FieldDecl* field : decl->fields()) manifest_.fields.push_back(field->getNameAsString());
    return true;
  }

private:
  Manifest& manifest_;
};

}  // namespace

void inspect_manifest(clang::ASTContext& ast, Manifest& manifest) {
  ManifestVisitor visitor(manifest);
  visitor.TraverseDecl(ast.getTranslationUnitDecl());
}

auto emit_contract(const Manifest& manifest) -> GeneratedContract {
  if (!manifest.exports.contains("c18_get_api") || manifest.fields.size() < 4) return {};
  return {
      "c18_get_api\nc18_api\n",
      "#include \"c18/abi.h\"\n"
      "_Static_assert(C18_ABI_VERSION == 1u, \"version\");\n"
      "_Static_assert(sizeof(c18_api) >= sizeof(void*) * 4, \"api table has function slots\");\n",
      "#include \"c18/abi.h\"\n"
      "static_assert(C18_ABI_VERSION == 1u);\n"
      "static_assert(sizeof(c18_api) >= sizeof(void*) * 4);\n",
  };
}

}  // namespace c18_tooling
