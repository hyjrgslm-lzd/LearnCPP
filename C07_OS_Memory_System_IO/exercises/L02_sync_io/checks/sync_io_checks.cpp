#include <sync_io.hpp>
#include <check.hpp>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

namespace {
std::filesystem::path temp_path(std::wstring name) {
    const auto stamp = std::to_string(
        std::chrono::steady_clock::now().time_since_epoch().count());
    return std::filesystem::temp_directory_path() / (L"c07_l02_" + std::wstring(stamp.begin(), stamp.end()) + L"_" + name);
}

void write_bytes(const std::filesystem::path& path, const std::string& data) {
    std::ofstream(path, std::ios::binary) << data;
}

std::string read_all(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    return {std::istreambuf_iterator<char>{in}, std::istreambuf_iterator<char>{}};
}
} // namespace

int main() {
    std::string data;
    for (int i = 0; i != 257; ++i) data.push_back(static_cast<char>(i & 0x7f));
    data += "tail bytes preserved";

    const auto src = temp_path(L"source data.txt");
    const auto dst = temp_path(L"target unicode \u7a7a\u683c.txt");
    write_bytes(src, data);
    const auto copied = c07_l02::copy_file_exact(src, dst, 17);
    check(copied.has_value(), "copy succeeds");
    check(*copied == data.size(), "copy reports every byte");
    check(read_all(dst) == data, "tail bytes preserved");

    const auto empty_src = temp_path(L"empty.txt");
    const auto empty_dst = temp_path(L"empty-out.txt");
    write_bytes(empty_src, {});
    const auto empty = c07_l02::copy_file_exact(empty_src, empty_dst, 5);
    check(empty && *empty == 0 && read_all(empty_dst).empty(), "empty file copies as EOF, not error");

    const auto existing = temp_path(L"existing.txt");
    write_bytes(existing, "sentinel");
    const auto refused = c07_l02::copy_file_exact(src, existing, 64);
    check(!refused, "target overwrite refused");
    check(read_all(existing) == "sentinel", "existing copy target preserved");
    check(!c07_l02::copy_file_exact(src, temp_path(L"bad-chunk.txt"), 0), "zero chunk rejected");

    std::error_code ignored;
    for (const auto& path : {src, dst, empty_src, empty_dst, existing}) std::filesystem::remove(path, ignored);
    std::cout << "L02 sync I/O checks passed\n";
}
