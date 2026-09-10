#include <mapping_lab.hpp>

#include <check.hpp>

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace {

std::filesystem::path temp_path(std::string name) {
    const auto stamp = std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    return std::filesystem::temp_directory_path() / ("c07_l04_" + stamp + "_" + name);
}

void write_text(const std::filesystem::path& path, const std::string& text) {
    std::ofstream(path, std::ios::binary) << text;
}

std::string read_text(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    return {std::istreambuf_iterator<char>{in}, std::istreambuf_iterator<char>{}};
}

} // namespace

int main() {
    const std::string source = "0123456789abcdefghijklmnopqrstuvwxyz-tail";
    const auto path = temp_path("source with spaces.txt");
    const auto empty = temp_path("empty.txt");
    write_text(path, source);
    write_text(empty, "");

    const auto granularity = c07::query_page_info().allocation_granularity;
    check(granularity != 0, "mapping granularity is available");
    const auto plan = c07_l04::plan_readonly_window(source.size(), 3, 11, granularity);
    check(plan.has_value(), "student returns a window plan");
    check(plan->aligned_offset == 0, "readonly mapping aligns view offset down");
    check(plan->delta == 3, "readonly mapping records delta from aligned view");
    check(plan->visible_length == 11, "readonly mapping records caller-visible length");
    check(plan->mapped_length == 14, "readonly mapping includes delta in native view length");

    auto mapping = c07_l04::map_window(path, 3, 11);
    check(mapping.has_value(), "map_window returns a real mapping owner");
    const auto bytes = mapping->bytes();
    const std::string mapped{reinterpret_cast<const char*>(bytes.data()), bytes.size()};
    check(mapped == source.substr(3, 11), "readonly mapping honors unaligned offsets");
    check(read_text(path) == source, "readonly fixture stays immutable");

    auto tail = c07_l04::map_window(path, source.size() - 5, 1000);
    check(tail.has_value(), "tail window maps successfully");
    const std::string tail_text{reinterpret_cast<const char*>(tail->bytes().data()), tail->bytes().size()};
    check(tail_text == source.substr(source.size() - 5), "mapping clamps requested length at EOF");

    check(!c07_l04::map_window(empty, 0, 1).has_value(), "empty file is rejected by map_window");
    check(!c07_l04::map_window(path, source.size(), 1).has_value(), "offset at or past EOF is rejected");

    const auto shared_path = temp_path("shared.txt");
    const auto private_path = temp_path("private.txt");
    write_text(shared_path, "abc");
    write_text(private_path, "abc");
    check(c07_l04::write_first_byte(shared_path, c07_l04::write_mapping_mode::shared, 'S').has_value(),
        "shared writable mapping operation succeeds");
    check(read_text(shared_path) == "Sbc", "shared mapping writes are visible through file readback");
    check(c07_l04::write_first_byte(private_path, c07_l04::write_mapping_mode::private_copy, 'P').has_value(),
        "private writable mapping operation succeeds");
    check(read_text(private_path) == "abc", "private mapping writes do not modify the file");

    std::error_code ignored;
    std::filesystem::remove(path, ignored);
    std::filesystem::remove(empty, ignored);
    std::filesystem::remove(shared_path, ignored);
    std::filesystem::remove(private_path, ignored);
    std::cout << "L04 mapping checks passed\n";
}
