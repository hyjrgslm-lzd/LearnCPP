#pragma once

#include "clang/AST/ASTContext.h"
#include "clang/AST/ASTTypeTraits.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Expr.h"
#include "clang/AST/Type.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Tooling/Core/Replacement.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/raw_ostream.h"

#include <set>
#include <string>
#include <tuple>
#include <vector>

namespace c18_tooling {

struct Finding {
  std::string file;
  unsigned line{};
  std::string text;
};

struct Manifest {
  std::set<std::string> exports;
  std::vector<std::string> fields;
};

struct GeneratedContract {
  std::string manifest;
  std::string c_consumer;
  std::string cpp_consumer;
};

inline auto location_text(const clang::SourceManager& sm, clang::SourceLocation loc) -> std::pair<std::string, unsigned> {
  clang::PresumedLoc presumed = sm.getPresumedLoc(sm.getSpellingLoc(loc));
  if (presumed.isInvalid()) return {"<invalid>", 0};
  return {presumed.getFilename(), presumed.getLine()};
}

inline auto type_text(clang::QualType type) -> std::string {
  std::string out;
  llvm::raw_string_ostream os(out);
  type.print(os, clang::PrintingPolicy(clang::LangOptions{}));
  return os.str();
}

inline auto is_opaque_context_pointer(clang::QualType type) -> bool {
  const auto* ptr = type->getAs<clang::PointerType>();
  if (ptr == nullptr) return false;
  clang::QualType pointee = ptr->getPointeeType().getCanonicalType();
  const auto* record = pointee->getAs<clang::RecordType>();
  return record != nullptr && record->getDecl()->getName() == "c18_context";
}

inline auto is_plain_abi_type(clang::QualType type) -> bool {
  clang::QualType canonical = type.getCanonicalType();
  if (canonical->isVoidType() || canonical->isBooleanType() || canonical->isIntegerType()) return true;
  if (canonical->isReferenceType()) return false;
  if (canonical->getAs<clang::TemplateSpecializationType>() != nullptr) return false;
  if (canonical->isFunctionPointerType()) return true;
  if (canonical->isPointerType()) {
    if (is_opaque_context_pointer(canonical)) return true;
    clang::QualType pointee = canonical->getPointeeType().getCanonicalType();
    return pointee->isVoidType() || pointee->isCharType() || pointee->isIntegerType() || pointee->isFunctionType() ||
           pointee->isRecordType();
  }
  const auto* record = canonical->getAs<clang::RecordType>();
  if (record == nullptr) return false;
  std::string name = record->getDecl()->getQualifiedNameAsString();
  return name == "c18_host_api" || name == "c18_api";
}

inline auto is_rewritable_user_loc(const clang::SourceManager& sm, clang::SourceLocation loc) -> bool {
  if (loc.isInvalid() || loc.isMacroID()) return false;
  clang::SourceLocation spelling = sm.getSpellingLoc(loc);
  if (sm.isInSystemHeader(spelling) || sm.isMacroArgExpansion(loc) || sm.isMacroBodyExpansion(loc)) return false;
  return true;
}

inline auto has_template_ancestor(clang::ASTContext& ast, const clang::DynTypedNode& node) -> bool {
  for (clang::DynTypedNode current = node;;) {
    if (const auto* decl = current.get<clang::Decl>()) {
      if (clang::isa<clang::FunctionTemplateDecl>(decl) || clang::isa<clang::ClassTemplateDecl>(decl) ||
          decl->getDeclContext()->isDependentContext()) {
        return true;
      }
    }
    auto parents = ast.getParents(current);
    if (parents.empty()) return false;
    current = parents[0];
  }
}

inline auto has_template_ancestor(clang::ASTContext& ast, const clang::Decl& decl) -> bool {
  return has_template_ancestor(ast, clang::DynTypedNode::create(decl));
}

inline auto has_template_ancestor(clang::ASTContext& ast, const clang::Stmt& stmt) -> bool {
  return has_template_ancestor(ast, clang::DynTypedNode::create(stmt));
}

inline auto add_replacement(clang::tooling::Replacements& replacements,
                            std::set<std::tuple<std::string, unsigned, unsigned, std::string>>& seen,
                            const clang::tooling::Replacement& replacement,
                            std::string& error_text) -> bool {
  auto key = std::make_tuple(replacement.getFilePath().str(), replacement.getOffset(), replacement.getLength(),
                             replacement.getReplacementText().str());
  if (!seen.insert(key).second) return true;
  llvm::Error error = replacements.add(replacement);
  if (!error) return true;
  error_text = llvm::toString(std::move(error));
  return false;
}

void inspect_abi(clang::ASTContext& ast, std::vector<Finding>& findings);
void register_rewrite_matchers(clang::ast_matchers::MatchFinder& finder,
                               clang::tooling::Replacements& replacements,
                               std::vector<Finding>& findings,
                               std::string old_symbol,
                               std::string new_symbol);
void inspect_manifest(clang::ASTContext& ast, Manifest& manifest);
auto emit_contract(const Manifest& manifest) -> GeneratedContract;

}  // namespace c18_tooling
