#include "output_guard.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

auto require(bool ok, const std::string& message) -> void {
  if (!ok) {
    std::cerr << message << "\n";
    std::exit(1);
  }
}

auto write_text(const fs::path& path, const std::string& text) -> void {
  fs::create_directories(path.parent_path());
  std::ofstream(path, std::ios::binary) << text;
}

auto temp_root() -> fs::path {
  fs::path root = fs::temp_directory_path() / ("c18-output-guard-" + std::to_string(std::rand()));
  fs::remove_all(root);
  fs::create_directories(root);
  return root;
}

}  // namespace

int main() {
  fs::path root = temp_root();
  fs::path a = root / "a" / "same.cpp";
  fs::path b = root / "b" / "same.cpp";
  write_text(a, "int a;\n");
  write_text(b, "int b;\n");

  std::vector<c18_output::RewriteOutput> rewrite_plan;
  std::string error =
      c18_output::validate_rewrite_outputs({a}, a.parent_path(), {a}, rewrite_plan);
  require(error.find("output equals input") != std::string::npos, "failed to reject output equal to input");

  fs::path out = root / "out";
  write_text(out / "same.cpp", "sentinel\n");
  error = c18_output::validate_rewrite_outputs({a}, out, {a}, rewrite_plan);
  require(error.find("output already exists") != std::string::npos, "failed to reject existing output");

  fs::remove(out / "same.cpp");
  error = c18_output::validate_rewrite_outputs({a, b}, out, {a, b}, rewrite_plan);
  require(error.find("duplicate output path") != std::string::npos, "failed to reject same-name collision");

  fs::path normalized = root / "dotdot" / ".." / "out" / "contract_check.c";
  write_text(normalized.lexically_normal(), "sentinel\n");
  std::vector<fs::path> outputs;
  error = c18_output::validate_named_outputs({}, root / "dotdot" / ".." / "out", {"contract_check.c"}, outputs);
  require(error.find("output already exists") != std::string::npos, "failed to reject normalized ../ output");

  fs::path link = out / "exports.txt";
  fs::remove(link);
  bool symlink_checked = false;
  try {
    fs::create_symlink(a, link);
    error = c18_output::validate_named_outputs({}, out, {"exports.txt"}, outputs);
    require(error.find("output already exists") != std::string::npos, "failed to reject symlink output");
    symlink_checked = true;
  } catch (const fs::filesystem_error& ex) {
    std::cout << "symlink check skipped: " << ex.code().message() << "\n";
  }

  fs::path fresh = out / "fresh.txt";
  error.clear();
  {
    auto stream = c18_output::open_new_text(fresh, error);
    require(stream.is_open(), "failed to create fresh output");
    stream << "fresh\n";
  }
  auto stream = c18_output::open_new_text(fresh, error);
  require(!stream.is_open(), "failed to enforce exclusive create");

  fs::remove_all(root);
  std::cout << "output guard PASS";
  if (symlink_checked) std::cout << " symlink";
  std::cout << "\n";
  return 0;
}
