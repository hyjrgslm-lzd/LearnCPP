#pragma once

#include <filesystem>
#include <fstream>
#include <set>
#include <string>
#include <vector>

namespace c18_output {

struct RewriteOutput {
  std::filesystem::path input;
  std::filesystem::path output;
};

inline auto normalized_absolute(const std::filesystem::path& path) -> std::filesystem::path {
  std::error_code ec;
  std::filesystem::path absolute = std::filesystem::absolute(path, ec);
  if (ec) absolute = path;
  return absolute.lexically_normal();
}

inline auto identity(const std::filesystem::path& path) -> std::filesystem::path {
  std::filesystem::path normalized = normalized_absolute(path);
  std::error_code ec;
  auto status = std::filesystem::symlink_status(normalized, ec);
  if (!ec && (std::filesystem::exists(status) || std::filesystem::is_symlink(status))) {
    auto canonical = std::filesystem::canonical(normalized, ec);
    if (!ec) return canonical.lexically_normal();
  }

  std::filesystem::path parent = normalized.parent_path();
  if (!parent.empty()) {
    auto canonical_parent = std::filesystem::canonical(parent, ec);
    if (!ec) return (canonical_parent / normalized.filename()).lexically_normal();
  }

  auto weak = std::filesystem::weakly_canonical(normalized, ec);
  if (!ec) return weak.lexically_normal();
  return normalized;
}

inline auto exists_or_symlink(const std::filesystem::path& path) -> bool {
  std::error_code ec;
  auto status = std::filesystem::symlink_status(path, ec);
  return !ec && (std::filesystem::exists(status) || std::filesystem::is_symlink(status));
}

inline auto validate_output(const std::set<std::filesystem::path>& input_ids,
                            std::set<std::filesystem::path>& output_ids,
                            const std::filesystem::path& output) -> std::string {
  std::filesystem::path output_id = identity(output);
  if (input_ids.contains(output_id)) return "output equals input: " + output.string();
  if (exists_or_symlink(output)) return "output already exists: " + output.string();
  if (!output_ids.insert(output_id).second) return "duplicate output path: " + output.string();
  return {};
}

inline auto input_id_set(const std::vector<std::filesystem::path>& inputs) -> std::set<std::filesystem::path> {
  std::set<std::filesystem::path> ids;
  for (const auto& input : inputs) ids.insert(identity(input));
  return ids;
}

inline auto validate_rewrite_outputs(const std::vector<std::filesystem::path>& inputs,
                                     const std::filesystem::path& out_dir,
                                     const std::vector<std::filesystem::path>& rewrite_files,
                                     std::vector<RewriteOutput>& plan) -> std::string {
  plan.clear();
  std::set<std::filesystem::path> input_ids = input_id_set(inputs);
  std::set<std::filesystem::path> output_ids;
  for (const auto& file : rewrite_files) {
    std::filesystem::path file_id = identity(file);
    if (!input_ids.contains(file_id)) return "refuse rewrite outside explicit input set: " + file.string();
    std::filesystem::path output = normalized_absolute(out_dir / file.filename());
    std::string error = validate_output(input_ids, output_ids, output);
    if (!error.empty()) return error;
    plan.push_back({file, output});
  }
  return {};
}

inline auto validate_named_outputs(const std::vector<std::filesystem::path>& inputs,
                                   const std::filesystem::path& out_dir,
                                   const std::vector<std::string>& names,
                                   std::vector<std::filesystem::path>& outputs) -> std::string {
  outputs.clear();
  std::set<std::filesystem::path> input_ids = input_id_set(inputs);
  std::set<std::filesystem::path> output_ids;
  for (const auto& name : names) {
    std::filesystem::path relative(name);
    if (relative.is_absolute() || relative.has_parent_path()) return "unsafe output name: " + name;
    std::filesystem::path output = normalized_absolute(out_dir / relative);
    std::string error = validate_output(input_ids, output_ids, output);
    if (!error.empty()) return error;
    outputs.push_back(output);
  }
  return {};
}

inline auto open_new_text(const std::filesystem::path& path, std::string& error) -> std::ofstream {
  std::error_code ec;
  if (!path.parent_path().empty()) std::filesystem::create_directories(path.parent_path(), ec);
  if (ec) {
    error = "cannot create output directory: " + path.parent_path().string() + ": " + ec.message();
    return {};
  }
  std::ofstream stream(path, std::ios::out | std::ios::binary | std::ios::noreplace);
  if (!stream.is_open()) error = "cannot create output exclusively: " + path.string();
  return stream;
}

}  // namespace c18_output
