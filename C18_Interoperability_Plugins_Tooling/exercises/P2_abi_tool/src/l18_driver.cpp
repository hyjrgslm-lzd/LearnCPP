#include "tooling_support.hpp"
#include "output_guard.hpp"

#include "clang/AST/ASTConsumer.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace {
llvm::cl::OptionCategory category("c18-l18 options");
llvm::cl::opt<std::string> out_dir("out-dir", llvm::cl::Required, llvm::cl::cat(category));

class Consumer : public clang::ASTConsumer {
public:
  explicit Consumer(c18_tooling::Manifest& manifest) : manifest_(manifest) {}
  void HandleTranslationUnit(clang::ASTContext& ast) override { c18_tooling::inspect_manifest(ast, manifest_); }

private:
  c18_tooling::Manifest& manifest_;
};

class Action : public clang::ASTFrontendAction {
public:
  explicit Action(c18_tooling::Manifest& manifest) : manifest_(manifest) {}
  auto CreateASTConsumer(clang::CompilerInstance&, llvm::StringRef) -> std::unique_ptr<clang::ASTConsumer> override {
    return std::make_unique<Consumer>(manifest_);
  }

private:
  c18_tooling::Manifest& manifest_;
};

class Factory : public clang::tooling::FrontendActionFactory {
public:
  explicit Factory(c18_tooling::Manifest& manifest) : manifest_(manifest) {}
  auto create() -> std::unique_ptr<clang::FrontendAction> override { return std::make_unique<Action>(manifest_); }

private:
  c18_tooling::Manifest& manifest_;
};

auto write_contract_files(const std::vector<std::string>& inputs, const c18_tooling::GeneratedContract& generated)
    -> bool {
  std::vector<std::filesystem::path> input_paths;
  for (const auto& input : inputs) input_paths.emplace_back(input);
  std::vector<std::filesystem::path> outputs;
  std::vector<std::string> names{"exports.txt", "contract_check.c", "contract_check.cpp"};
  std::string error = c18_output::validate_named_outputs(input_paths, out_dir.getValue(), names, outputs);
  if (!error.empty()) {
    llvm::errs() << error << "\n";
    return false;
  }
  std::vector<std::string> texts{generated.manifest, generated.c_consumer, generated.cpp_consumer};
  for (std::size_t i = 0; i < outputs.size(); ++i) {
    std::string open_error;
    auto os = c18_output::open_new_text(outputs[i], open_error);
    if (!os.is_open()) {
      llvm::errs() << open_error << "\n";
      return false;
    }
    os << texts[i];
  }
  return true;
}
}  // namespace

int main(int argc, const char** argv) {
  auto parser = clang::tooling::CommonOptionsParser::create(argc, argv, category);
  if (!parser) {
    llvm::errs() << llvm::toString(parser.takeError()) << "\n";
    return 2;
  }
  c18_tooling::Manifest manifest;
  clang::tooling::ClangTool tool(parser->getCompilations(), parser->getSourcePathList());
  Factory factory(manifest);
  if (tool.run(&factory) != 0) return 1;
  c18_tooling::GeneratedContract generated = c18_tooling::emit_contract(manifest);
  if (generated.manifest.empty() || generated.c_consumer.empty() || generated.cpp_consumer.empty()) {
    llvm::errs() << "generator missed manifest or contract files\n";
    return 1;
  }
  if (!write_contract_files(parser->getSourcePathList(), generated)) {
    return 1;
  }
  return 0;
}
