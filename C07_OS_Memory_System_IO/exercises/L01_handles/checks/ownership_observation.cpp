#include <c07/os.hpp>
#include <check.hpp>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace {
std::filesystem::path temp_path() {
    const auto stamp = std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    return std::filesystem::temp_directory_path() / ("c07_l01_owner_" + stamp + ".txt");
}

bool can_remove(const std::filesystem::path& path) {
    std::error_code ignored;
    return std::filesystem::remove(path, ignored);
}
} // namespace

int main() {
    const auto path = temp_path();
    { std::ofstream(path, std::ios::binary) << "owner"; }
    auto opened = c07::open_existing_file(path);
    check(opened.has_value(), "open existing file for ownership observation");
    c07::unique_file owner = std::move(*opened);
    c07::unique_file moved = std::move(owner);
    check(!owner && moved, "move transfers release responsibility");
    auto raw = moved.release();
    check(!moved, "release leaves wrapper empty");
    c07::unique_file rebound{raw};
    rebound.reset();
    rebound.reset();
    check(can_remove(path), "repeated reset does not keep or double-close the file");
    std::cout << "L01 ownership observation passed\n";
}
