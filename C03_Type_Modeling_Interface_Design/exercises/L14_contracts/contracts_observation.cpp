#include <check.hpp>

#include <charconv>
#include <expected>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>
#include <version>

namespace {

std::expected<int, std::string> parse_port(std::string_view text)
{
    int value = 0;
    const char* first = text.data();
    const char* last = text.data() + text.size();
    const auto [ptr, ec] = std::from_chars(first, last, value);
    if (ec != std::errc{} || ptr != last) {
        return std::unexpected("port is not an integer");
    }
    if (value < 1 || value > 65535) {
        return std::unexpected("port is outside 1..65535");
    }
    return value;
}

std::expected<std::string, std::string> build_endpoint(std::string_view host, int validated_port)
{
    if (host.empty()) {
        return std::unexpected("host is empty");
    }
    if (validated_port < 1 || validated_port > 65535) {
        return std::unexpected("validated_port precondition failed in safe model");
    }
    return std::string(host) + ':' + std::to_string(validated_port);
}

void print_capability_macros()
{
#ifdef _MSVC_STL_HARDENING
    std::cout << "_MSVC_STL_HARDENING=" << _MSVC_STL_HARDENING << '\n';
#else
    std::cout << "_MSVC_STL_HARDENING=not-defined\n";
#endif
#ifdef __cpp_contracts
    std::cout << "__cpp_contracts=" << __cpp_contracts << '\n';
#else
    std::cout << "__cpp_contracts=not-defined\n";
#endif
#ifdef __cpp_lib_contracts
    std::cout << "__cpp_lib_contracts=" << __cpp_lib_contracts << '\n';
#else
    std::cout << "__cpp_lib_contracts=not-defined\n";
#endif
    std::cout << "pattern_matching=P2688 proposal tracking only\n";
}

} // namespace

int main()
{
    auto good = parse_port("443");
    check(good.has_value(), "valid port parses");
    check(*good == 443, "parsed port value is preserved");

    auto bad_text = parse_port("443x");
    check(!bad_text.has_value(), "invalid text is rejected through expected");

    auto bad_range = parse_port("70000");
    check(!bad_range.has_value(), "out of range port is rejected through expected");

    auto endpoint = build_endpoint("example.test", *good);
    check(endpoint.has_value(), "validated input enters internal model");
    check(*endpoint == "example.test:443", "endpoint uses validated port");

    auto rejected_internal = build_endpoint("example.test", 0);
    check(!rejected_internal.has_value(), "safe model reports precondition breach without UB");

    print_capability_macros();
}
