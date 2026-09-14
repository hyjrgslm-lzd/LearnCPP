#include "tooling_support.hpp"
#include "output_guard.hpp"

#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Core/Replacement.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/raw_ostream.h"

#include <map>
#include <filesystem>
#include <string>
#include <vector>

namespace {
llvm::cl::OptionCategory category("c18-l17 options");
llvm::cl::opt<std::string> old_symbol("old-symbol", llvm::cl::init("c18_process_old"), llvm::cl::cat(category));
llvm::cl::opt<std::string> new_symbol("new-symbol", llvm::cl::init("c18_process"), llvm::cl::cat(category));
llvm::cl::opt<std::string> out_dir("out-dir", llvm::cl::init(""), llvm::cl::cat(category));
}  // namespace

int main(int argc, const char** argv) {
  auto parser = clang::tooling::CommonOptionsParser::create(argc, argv, category);
  if (!parser) {
    llvm::errs() << llvm::toString(parser.takeError()) << "\n";
    return 2;
  }
  clang::tooling::Replacements replacements;
  std::vector<c18_tooling::Finding> findings;
  clang::ast_matchers::MatchFinder finder;
  c18_tooling::register_rewrite_matchers(finder, replacements, findings, old_symbol, new_symbol);
  struct ConsumerFactory {
    clang::ast_matchers::MatchFinder& finder;
    auto newASTConsumer() -> std::unique_ptr<clang::ASTConsumer> { return finder.newASTConsumer(); }
  } consumer_factory{finder};
  clang::tooling::ClangTool tool(parser->getCompilations(), parser->getSourcePathList());
  int rc = tool.run(clang::tooling::newFrontendActionFactory(&consumer_factory).get());
  for (const auto& finding : findings) {
    llvm::errs() << finding.file << ":" << finding.line << ": " << finding.text << "\n";
  }
  if (rc != 0 || !findings.empty()) return 1;
  if (out_dir.empty()) {
    for (const auto& replacement : replacements) {
      llvm::outs() << replacement.getFilePath() << ":" << replacement.getOffset() << ": replace "
                   << replacement.getLength() << " with " << replacement.getReplacementText() << "\n";
    }
    return 0;
  }
  std::map<std::string, clang::tooling::Replacements> by_file;
  for (const auto& replacement : replacements) {
    llvm::Error error = by_file[replacement.getFilePath().str()].add(replacement);
    if (error) {
      llvm::errs() << "replacement conflict while grouping: " << llvm::toString(std::move(error)) << "\n";
      return 1;
    }
  }
  std::vector<std::filesystem::path> inputs;
  for (const auto& path : parser->getSourcePathList()) inputs.emplace_back(path);
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
    auto buffer = llvm::MemoryBuffer::getFile(file);
    if (!buffer) {
      llvm::errs() << "cannot read " << file << "\n";
      return 1;
    }
    auto edited = clang::tooling::applyAllReplacements(buffer.get()->getBuffer(), reps);
    if (!edited) {
      llvm::errs() << llvm::toString(edited.takeError()) << "\n";
      return 1;
    }
    std::string open_error;
    auto os = c18_output::open_new_text(item.output, open_error);
    if (!os.is_open()) {
      llvm::errs() << open_error << "\n";
      return 1;
    }
    os << *edited;
  }
  return 0;
}
