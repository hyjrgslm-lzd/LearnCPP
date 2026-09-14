#include "c18/abi.h"
#include "output_guard.hpp"

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTTypeTraits.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/CompilationDatabase.h"
#include "clang/Tooling/Core/Replacement.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/raw_ostream.h"

#include <filesystem>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::tooling;

namespace {

llvm::cl::OptionCategory category("c18-tool options");
llvm::cl::opt<std::string> subcommand("c18-subcommand", llvm::cl::Required,
                                      llvm::cl::desc("check, rewrite, or generate"), llvm::cl::cat(category));
llvm::cl::opt<std::string> old_symbol("old-symbol", llvm::cl::init("c18_process_old"), llvm::cl::cat(category));
llvm::cl::opt<std::string> new_symbol("new-symbol", llvm::cl::init("c18_process"), llvm::cl::cat(category));
llvm::cl::opt<std::string> out_dir("out-dir", llvm::cl::init(""), llvm::cl::cat(category));

struct Finding {
  std::string file;
  unsigned line{};
  std::string text;
};

struct Manifest {
  std::set<std::string> exports;
  std::vector<std::string> fields;
};

auto location_text(const SourceManager& sm, SourceLocation loc) -> std::pair<std::string, unsigned> {
  PresumedLoc presumed = sm.getPresumedLoc(sm.getSpellingLoc(loc));
  if (presumed.isInvalid()) return {"<invalid>", 0};
  return {presumed.getFilename(), presumed.getLine()};
}

auto is_rewritable_user_loc(const SourceManager& sm, SourceLocation loc) -> bool {
  if (loc.isInvalid() || loc.isMacroID()) return false;
  SourceLocation spelling = sm.getSpellingLoc(loc);
  if (sm.isInSystemHeader(spelling) || sm.isMacroArgExpansion(loc) || sm.isMacroBodyExpansion(loc)) return false;
  return true;
}

auto has_template_ancestor(ASTContext& ast, const DynTypedNode& node) -> bool {
  for (DynTypedNode current = node;;) {
    if (const auto* decl = current.get<Decl>()) {
      if (isa<FunctionTemplateDecl>(decl) || isa<ClassTemplateDecl>(decl) || decl->getDeclContext()->isDependentContext()) {
        return true;
      }
    }
    if (const auto* stmt = current.get<Stmt>()) {
      if (isa<SubstNonTypeTemplateParmExpr>(stmt) || isa<CXXDependentScopeMemberExpr>(stmt)) return true;
    }
    auto parents = ast.getParents(current);
    if (parents.empty()) return false;
    current = parents[0];
  }
}

auto has_template_ancestor(ASTContext& ast, const Decl& decl) -> bool {
  return has_template_ancestor(ast, DynTypedNode::create(decl));
}

auto has_template_ancestor(ASTContext& ast, const Stmt& stmt) -> bool {
  return has_template_ancestor(ast, DynTypedNode::create(stmt));
}

auto type_text(QualType type) -> std::string {
  std::string out;
  llvm::raw_string_ostream os(out);
  type.print(os, PrintingPolicy(LangOptions{}));
  return os.str();
}

auto is_opaque_context_pointer(QualType type) -> bool {
  const auto* ptr = type->getAs<PointerType>();
  if (ptr == nullptr) return false;
  QualType pointee = ptr->getPointeeType().getCanonicalType();
  const auto* record = pointee->getAs<RecordType>();
  return record != nullptr && record->getDecl()->getName() == "c18_context";
}

auto is_plain_abi_type(QualType type) -> bool {
  QualType canonical = type.getCanonicalType();
  if (canonical->isVoidType() || canonical->isBooleanType() || canonical->isIntegerType()) return true;
  if (canonical->isPointerType()) {
    if (is_opaque_context_pointer(canonical)) return true;
    QualType pointee = canonical->getPointeeType().getCanonicalType();
    return pointee->isVoidType() || pointee->isCharType() || pointee->isIntegerType() || pointee->isFunctionType() ||
           pointee->isRecordType();
  }
  if (canonical->isFunctionPointerType()) return true;
  if (canonical->isReferenceType()) return false;
  if (canonical->getAs<TemplateSpecializationType>() != nullptr) return false;
  const auto* record = canonical->getAs<RecordType>();
  if (record == nullptr) return false;
  std::string name = record->getDecl()->getQualifiedNameAsString();
  return name == "c18_host_api" || name == "c18_api";
}

class AbiCheckVisitor : public RecursiveASTVisitor<AbiCheckVisitor> {
public:
  explicit AbiCheckVisitor(ASTContext& ast) : ast_(ast) {}

  bool shouldVisitTemplateInstantiations() const { return true; }
  bool shouldVisitImplicitCode() const { return false; }

  bool VisitFunctionDecl(FunctionDecl* decl) {
    if (!decl->isExternC()) return true;
    if (!decl->getName().starts_with("c18_")) return true;
    check_type(decl->getReturnType(), decl->getLocation(), "return type");
    for (ParmVarDecl* param : decl->parameters()) {
      check_type(param->getType(), param->getLocation(), "parameter type");
    }
    return true;
  }

  bool VisitFieldDecl(FieldDecl* field) {
    const auto* parent = dyn_cast<RecordDecl>(field->getDeclContext());
    if (parent == nullptr || parent->getName() != "c18_api") return true;
    check_type(field->getType(), field->getLocation(), "function-table field");
    return true;
  }

  const std::vector<Finding>& findings() const { return findings_; }

private:
  void check_type(QualType type, SourceLocation loc, StringRef role) {
    std::string printed = type_text(type);
    if (printed.find("std::") != std::string::npos || printed.find("class ") != std::string::npos ||
        printed.find("basic_string") != std::string::npos || !is_plain_abi_type(type)) {
      auto [file, line] = location_text(ast_.getSourceManager(), loc);
      findings_.push_back({file, line, "dangerous exported ABI type in " + role.str() + ": " + printed});
    }
  }

  ASTContext& ast_;
  std::vector<Finding> findings_;
};

class CheckConsumer : public ASTConsumer {
public:
  explicit CheckConsumer(ASTContext& ast, std::vector<Finding>& output) : visitor_(ast), output_(output) {}
  void HandleTranslationUnit(ASTContext& ast) override {
    visitor_.TraverseDecl(ast.getTranslationUnitDecl());
    output_.insert(output_.end(), visitor_.findings().begin(), visitor_.findings().end());
  }

private:
  AbiCheckVisitor visitor_;
  std::vector<Finding>& output_;
};

class CheckAction : public ASTFrontendAction {
public:
  explicit CheckAction(std::vector<Finding>& findings) : findings_(findings) {}
  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance& ci, StringRef) override {
    return std::make_unique<CheckConsumer>(ci.getASTContext(), findings_);
  }

private:
  std::vector<Finding>& findings_;
};

class CheckFactory : public FrontendActionFactory {
public:
  explicit CheckFactory(std::vector<Finding>& findings) : findings_(findings) {}
  std::unique_ptr<FrontendAction> create() override { return std::make_unique<CheckAction>(findings_); }

private:
  std::vector<Finding>& findings_;
};

class RewriteCallback : public MatchFinder::MatchCallback {
public:
  explicit RewriteCallback(Replacements& replacements) : replacements_(replacements) {}

  void run(const MatchFinder::MatchResult& result) override {
    const auto* ref = result.Nodes.getNodeAs<DeclRefExpr>("ref");
    const auto* decl = result.Nodes.getNodeAs<NamedDecl>("decl");
    if (ref == nullptr && decl == nullptr) return;
    const SourceManager& sm = *result.SourceManager;
    ASTContext& ast = *result.Context;
    SourceLocation loc = ref != nullptr ? ref->getLocation() : decl->getLocation();
    if (!is_rewritable_user_loc(sm, loc)) {
      auto [file, line] = location_text(sm, loc);
      refused_.push_back({file, line, "refuse macro/template/system-header rewrite"});
      return;
    }
    if ((decl != nullptr && has_template_ancestor(ast, *decl)) || (ref != nullptr && has_template_ancestor(ast, *ref))) {
      auto [file, line] = location_text(sm, loc);
      refused_.push_back({file, line, "refuse macro/template/system-header rewrite"});
      return;
    }
    StringRef file = sm.getFilename(sm.getSpellingLoc(loc));
    unsigned offset = sm.getFileOffset(sm.getSpellingLoc(loc));
    llvm::Error error = replacements_.add(Replacement(file, offset, old_symbol.size(), new_symbol));
    if (error) {
      auto [where, line] = location_text(sm, loc);
      refused_.push_back({where, line, "replacement conflict: " + llvm::toString(std::move(error))});
    }
  }

  std::vector<Finding> refused_;

private:
  Replacements& replacements_;
};

class RewriteConsumerFactory {
public:
  RewriteConsumerFactory(MatchFinder& finder) : finder_(finder) {}
  std::unique_ptr<ASTConsumer> newASTConsumer() { return finder_.newASTConsumer(); }

private:
  MatchFinder& finder_;
};

class GenerateVisitor : public RecursiveASTVisitor<GenerateVisitor> {
public:
  GenerateVisitor(ASTContext& ast, Manifest& manifest) : ast_(ast), manifest_(manifest) {}
  bool VisitFunctionDecl(FunctionDecl* decl) {
    if (decl->getName() == "c18_get_api") {
      manifest_.exports.insert("c18_get_api");
    }
    return true;
  }
  bool VisitRecordDecl(RecordDecl* decl) {
    if (decl->getName() == "c18_api" && decl->isCompleteDefinition()) {
      for (FieldDecl* field : decl->fields()) manifest_.fields.push_back(field->getNameAsString());
    }
    return true;
  }

private:
  ASTContext& ast_;
  Manifest& manifest_;
};

class GenerateConsumer : public ASTConsumer {
public:
  explicit GenerateConsumer(ASTContext& ast, GenerateVisitor& visitor) : visitor_(visitor) {}
  void HandleTranslationUnit(ASTContext& ast) override { visitor_.TraverseDecl(ast.getTranslationUnitDecl()); }

private:
  GenerateVisitor& visitor_;
};

class GenerateAction : public ASTFrontendAction {
public:
  explicit GenerateAction(Manifest& manifest) : manifest_(manifest) {}
  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance& ci, StringRef) override {
    owned_ = std::make_unique<GenerateVisitor>(ci.getASTContext(), manifest_);
    return std::make_unique<GenerateConsumer>(ci.getASTContext(), *owned_);
  }

private:
  Manifest& manifest_;
  std::unique_ptr<GenerateVisitor> owned_;
};

auto run_check(CompilationDatabase& db, ArrayRef<std::string> files) -> int {
  std::vector<Finding> findings;
  ClangTool tool(db, files);
  CheckFactory factory(findings);
  int rc = tool.run(&factory);
  for (const Finding& finding : findings) {
    llvm::outs() << finding.file << ":" << finding.line << ": " << finding.text << "\n";
  }
  return rc == 0 && findings.empty() ? 0 : 1;
}

auto run_rewrite(CompilationDatabase& db, ArrayRef<std::string> files) -> int {
  DeclarationMatcher old_decl = namedDecl(hasName(old_symbol)).bind("decl");
  StatementMatcher old_ref = declRefExpr(to(namedDecl(hasName(old_symbol)))).bind("ref");
  Replacements replacements;
  RewriteCallback callback(replacements);
  MatchFinder finder;
  finder.addMatcher(old_decl, &callback);
  finder.addMatcher(old_ref, &callback);
  RewriteConsumerFactory consumers(finder);
  ClangTool tool(db, files);
  int rc = tool.run(newFrontendActionFactory(&consumers).get());
  for (const Finding& refused : callback.refused_) {
    llvm::errs() << refused.file << ":" << refused.line << ": " << refused.text << "\n";
  }
  if (!callback.refused_.empty()) return 1;
  if (out_dir.empty()) {
    for (const Replacement& replacement : replacements) {
      llvm::outs() << replacement.getFilePath() << ":" << replacement.getOffset() << ": replace "
                   << replacement.getLength() << " with " << replacement.getReplacementText() << "\n";
    }
    return rc;
  }
  std::map<std::string, Replacements> by_file;
  for (const Replacement& replacement : replacements) {
    llvm::Error error = by_file[replacement.getFilePath()].add(replacement);
    if (error) {
      llvm::errs() << "replacement conflict while grouping: " << llvm::toString(std::move(error)) << "\n";
      return 1;
    }
  }
  std::vector<std::filesystem::path> inputs;
  for (const auto& path : files) inputs.emplace_back(path);
  std::vector<std::filesystem::path> rewrite_files;
  for (const auto& [file, _] : by_file) rewrite_files.emplace_back(file);
  std::vector<c18_output::RewriteOutput> output_plan;
  std::string output_error = c18_output::validate_rewrite_outputs(inputs, out_dir.getValue(), rewrite_files, output_plan);
  if (!output_error.empty()) {
    llvm::errs() << output_error << "\n";
    return 1;
  }
  for (const auto& item : output_plan) {
    std::string file = item.input.string();
    auto found = by_file.find(file);
    if (found == by_file.end()) {
      llvm::errs() << "internal rewrite output planning error for " << file << "\n";
      return 1;
    }
    const auto& reps = found->second;
    llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>> buffer = llvm::MemoryBuffer::getFile(file);
    if (!buffer) return 1;
    auto edited = applyAllReplacements(buffer.get()->getBuffer(), reps);
    if (!edited) return 1;
    std::string open_error;
    auto os = c18_output::open_new_text(item.output, open_error);
    if (!os.is_open()) {
      llvm::errs() << open_error << "\n";
      return 1;
    }
    os << *edited;
  }
  return rc;
}

auto run_generate(CompilationDatabase& db, ArrayRef<std::string> files) -> int {
  Manifest manifest;
  class Factory : public FrontendActionFactory {
  public:
    explicit Factory(Manifest& manifest) : manifest_(manifest) {}
    std::unique_ptr<FrontendAction> create() override { return std::make_unique<GenerateAction>(manifest_); }
    Manifest& manifest_;
  } factory(manifest);
  ClangTool tool(db, files);
  int rc = tool.run(&factory);
  if (rc != 0) return 1;
  std::string manifest_text = "c18_get_api\nc18_api\n";
  std::string contract_c = "#include \"c18/abi.h\"\n"
                           "_Static_assert(C18_ABI_VERSION == 1u, \"version\");\n"
                           "_Static_assert(sizeof(c18_api) >= sizeof(void*) * 4, \"api table has function slots\");\n";
  std::string contract_cpp = "#include \"c18/abi.h\"\n"
                             "static_assert(C18_ABI_VERSION == 1u);\n"
                             "static_assert(sizeof(c18_api) >= sizeof(void*) * 4);\n";
  if (!out_dir.empty()) {
    std::vector<std::filesystem::path> inputs;
    for (const auto& path : files) inputs.emplace_back(path);
    std::vector<std::string> names{"exports.txt", "contract_check.c", "contract_check.cpp"};
    std::vector<std::filesystem::path> outputs;
    std::string output_error = c18_output::validate_named_outputs(inputs, out_dir.getValue(), names, outputs);
    if (!output_error.empty()) {
      llvm::errs() << output_error << "\n";
      return 1;
    }
    std::vector<std::string> texts{manifest_text, contract_c, contract_cpp};
    for (std::size_t i = 0; i < outputs.size(); ++i) {
      std::string open_error;
      auto os = c18_output::open_new_text(outputs[i], open_error);
      if (!os.is_open()) {
        llvm::errs() << open_error << "\n";
        return 1;
      }
      os << texts[i];
    }
  }
  llvm::outs() << "exports:\n" << manifest_text;
  llvm::outs() << "contract_check_c:\n" << contract_c;
  llvm::outs() << "contract_check_cpp:\n" << contract_cpp;
  return 0;
}

}  // namespace

int main(int argc, const char** argv) {
  auto parser = CommonOptionsParser::create(argc, argv, category);
  if (!parser) {
    llvm::errs() << llvm::toString(parser.takeError()) << "\n";
    return 2;
  }
  CompilationDatabase& db = parser->getCompilations();
  const std::vector<std::string>& files = parser->getSourcePathList();
  if (subcommand == "check") return run_check(db, files);
  if (subcommand == "rewrite") return run_rewrite(db, files);
  if (subcommand == "generate") return run_generate(db, files);
  llvm::errs() << "unknown c18-subcommand: " << subcommand << "\n";
  return 2;
}
