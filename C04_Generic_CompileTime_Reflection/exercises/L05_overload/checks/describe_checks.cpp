#include <check.hpp>
#include <describe.hpp>

#include <array>
#include <concepts>
#include <iostream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace c04_l05_fixture {

struct MemberDescribed {
    int calls{0};

    int describe() noexcept {
        ++calls;
        return calls;
    }
};

struct BothMemberAndRange {
    int calls{0};
    int values[2]{1, 2};

    int describe() noexcept {
        ++calls;
        return calls;
    }

    int* begin() noexcept { return values; }
    int* end() noexcept { return values + 2; }
};

struct NoDescription {};

} // namespace c04_l05_fixture

namespace c04_l05_checks {

template<class T>
concept c04_describable = requires(T&& object) {
    c04_overload::describe(std::forward<T>(object));
};

template<class T>
using describe_result_t = decltype(c04_overload::describe(std::declval<T>()));

template<class Result>
void check_literal_extent(const Result& literal) {
    (void)literal;
    if constexpr (std::same_as<Result, c04_overload::literal_result<4>>) {
        check(Result::extent == 4, "literal extent includes the null terminator");
    } else {
        check(false, "string literal overload must keep the array extent");
    }
}

template<class Result>
void check_member_result(const Result& result, int original_calls) {
    if constexpr (std::same_as<Result, c04_overload::member_result>) {
        check(result.calls == original_calls + 1, "member describe is actually called");
    } else {
        check(false, "member describe returns member_result");
    }
}

inline void check_member_path() {
    c04_l05_fixture::MemberDescribed object{};
    auto result = c04_overload::describe(object);
    check_member_result(result, 0);
    check(object.calls == 1, "member describe mutates the original object");

    c04_l05_fixture::BothMemberAndRange both{};
    auto both_result = c04_overload::describe(both);
    check_member_result(both_result, 0);
    check(both.calls == 1, "member path is used before range fallback");
}

inline void check_literal_integral_range() {
    auto literal = c04_overload::describe("abc");
    check_literal_extent(literal);

    auto integral = c04_overload::describe(42);
    check((std::same_as<decltype(integral), c04_overload::integral_result>),
        "int chooses the integral overload");

    std::vector<int> values{1, 2, 3};
    auto range = c04_overload::describe(values);
    check((std::same_as<decltype(range), c04_overload::range_result>),
        "range fallback handles ordinary ranges");
}

inline void check_rejections() {
    check(!c04_describable<bool>, "bool is not accepted by the integral overload");
    check(!c04_describable<c04_l05_fixture::NoDescription&>,
        "no-description object has no viable overload");
}

} // namespace c04_l05_checks

int main() {
    c04_l05_checks::check_member_path();
    c04_l05_checks::check_literal_integral_range();
    c04_l05_checks::check_rejections();
    std::cout << "L05_overload checks OK\n";
}
