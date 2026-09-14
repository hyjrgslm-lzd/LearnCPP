#include "tooling_support.hpp"

#include "clang/AST/ASTConsumer.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"

#include <memory>
#include <string>
#include <vector>

namespace {
llvm::cl::OptionCategory category("c18-l16 options");

class Consumer : public clang::ASTConsumer {
public:
  explicit Consumer(std::vector<c18_tooling::Finding>& findings) : findings_(findings) {}
  void HandleTranslationUnit(clang::ASTContext& ast) override { c18_tooling::inspect_abi(ast, findings_); }

private:
  std::vector<c18_tooling::Finding>& findings_;
};

class Action : public clang::ASTFrontendAction {
public:
  explicit Action(std::vector<c18_tooling::Finding>& findings) : findings_(findings) {}
  auto CreateASTConsumer(clang::CompilerInstance&, llvm::StringRef) -> std::unique_ptr<clang::ASTConsumer> override {
    return std::make_unique<Consumer>(findings_);
  }

private:
  std::vector<c18_tooling::Finding>& findings_;
};

class Factory : public clang::tooling::FrontendActionFactory {
public:
  explicit Factory(std::vector<c18_tooling::Finding>& findings) : findings_(findings) {}
  auto create() -> std::unique_ptr<clang::FrontendAction> override { return std::make_unique<Action>(findings_); }

private:
  std::vector<c18_tooling::Finding>& findings_;
};
}  // namespace

int main(int argc, const char** argv) {
  auto parser = clang::tooling::CommonOptionsParser::create(argc, argv, category);
  if (!parser) {
    llvm::errs() << llvm::toString(parser.takeError()) << "\n";
    return 2;
  }
  std::vector<c18_tooling::Finding> findings;
  clang::tooling::ClangTool tool(parser->getCompilations(), parser->getSourcePathList());
  Factory factory(findings);
  int rc = tool.run(&factory);
  for (const auto& finding : findings) {
    llvm::outs() << finding.file << ":" << finding.line << ": " << finding.text << "\n";
  }
  return rc == 0 && findings.empty() ? 0 : 1;
}
