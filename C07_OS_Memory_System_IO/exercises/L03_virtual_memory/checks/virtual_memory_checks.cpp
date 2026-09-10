#include <virtual_memory_lab.hpp>

#include <check.hpp>

#include <c07/memory.hpp>

#include <cstddef>
#include <cstdint>
#include <expected>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace {

bool power_of_two(std::size_t value) noexcept {
    return value != 0 && (value & (value - 1)) == 0;
}

bool readable_page(void* address) {
#ifdef _WIN32
    MEMORY_BASIC_INFORMATION info{};
    if (::VirtualQuery(address, &info, sizeof(info)) == 0) return false;
    const DWORD readable = PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY
        | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY;
    return info.State == MEM_COMMIT && (info.Protect & readable) != 0 && (info.Protect & PAGE_GUARD) == 0;
#else
    std::ifstream maps("/proc/self/maps");
    std::string line;
    const auto target = reinterpret_cast<std::uintptr_t>(address);
    while (std::getline(maps, line)) {
        std::istringstream in(line);
        std::string range;
        std::string perms;
        if (!(in >> range >> perms)) continue;
        const auto dash = range.find('-');
        if (dash == std::string::npos) continue;
        const auto first = std::stoull(range.substr(0, dash), nullptr, 16);
        const auto last = std::stoull(range.substr(dash + 1), nullptr, 16);
        if (target >= first && target < last) return !perms.empty() && perms[0] == 'r';
    }
    return false;
#endif
}

} // namespace

int main() {
    const auto info = c07::query_page_info();
    check(power_of_two(info.page_size), "page size is a power of two");

    auto made = c07_l03::make_region(2, std::byte{0x5a});
    check(made.has_value(), "make_region returns a real virtual_region");
    auto region = std::move(*made);
    check(region.data() != nullptr && region.size() == info.page_size * 2, "region reserves requested page count");
    check(readable_page(region.data()), "first page is committed and readable");
    auto first = region.bytes(0, info.page_size);
    check(first.size() == info.page_size, "region exposes the committed first page");
    check(first.front() == std::byte{0x5a} && first.back() == std::byte{0x5a}, "committed page keeps written bytes");
    check(region.protect(0, info.page_size, c07::page_access::read_only).has_value(),
        "protect changes access state without ending the region");
    check(readable_page(region.data()), "protected read-only page remains safely readable");
    check(region.decommit(0, info.page_size).has_value(), "decommit removes page access while preserving the reservation");
    check(!readable_page(region.data()), "decommitted page is not read before recommit");
    check(region.commit(0, info.page_size, c07::page_access::read_write).has_value(), "decommitted page can be recommitted");
    check(readable_page(region.data()), "recommitted page is readable again");
    check(!c07_l03::make_region(0, std::byte{0x11}).has_value(), "zero page count is rejected");
    check(!region.commit(region.size(), info.page_size).has_value(), "out-of-range operation is rejected");
    check(!region.commit(0, 0).has_value(), "zero-length operation is rejected");
    std::cout << "L03 virtual memory checks passed\n";
}
