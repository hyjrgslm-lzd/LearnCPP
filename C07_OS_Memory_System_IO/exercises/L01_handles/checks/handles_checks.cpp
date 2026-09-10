#include <handles.hpp>
#include <check.hpp>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace {
std::filesystem::path temp_path(std::string name) {
    const auto stamp = std::to_string(
        std::chrono::steady_clock::now().time_since_epoch().count());
    return std::filesystem::temp_directory_path() / ("c07_l01_" + stamp + "_" + name);
}

std::string read_all(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    return {std::istreambuf_iterator<char>{in}, std::istreambuf_iterator<char>{}};
}
} // namespace

int main() {
    const std::string payload = "handle ownership: one file, one close, exact bytes\n";
    const auto path = temp_path("note.txt");
    const auto result = c07_l01::create_note(path, std::span<const char>{payload.data(), payload.size()});
    check(result.has_value(), "create_note succeeds for a fresh path");
    check(*result == payload.size(), "byte count reports the payload size");
    check(read_all(path) == payload, "file contains exactly the requested payload");

    const std::string sentinel = "do not overwrite";
    const auto existing = temp_path("existing.txt");
    { std::ofstream(existing, std::ios::binary) << sentinel; }
    const auto refused = c07_l01::create_note(existing, std::span<const char>{payload.data(), payload.size()});
    check(!refused.has_value(), "existing target is refused");
    check(read_all(existing) == sentinel, "preexisting file preserved");

    std::error_code ignored;
    std::filesystem::remove(path, ignored);
    std::filesystem::remove(existing, ignored);
    std::cout << "L01 handle checks passed\n";
}
