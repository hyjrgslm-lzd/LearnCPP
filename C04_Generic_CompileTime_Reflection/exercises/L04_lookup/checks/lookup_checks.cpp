#include <check.hpp>
#include <lookup_probe.hpp>

#include <concepts>
#include <iostream>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace c04_l04_fixture {

struct MemberOnly {
    int value{10};

    int& inspect() & noexcept { return value; }
    const int& inspect() const& noexcept { return value; }
    int&& inspect() && noexcept { return std::move(value); }
};

struct AdlOnly {
    int value{20};
};

int& inspect(AdlOnly& object) noexcept {
    return object.value;
}

const int& inspect(const AdlOnly& object) noexcept {
    return object.value;
}

struct HiddenFriend {
    int value{30};

    friend int& inspect(HiddenFriend& object) noexcept {
        return object.value;
    }
};

struct BothRoutes {
    int member_value{40};
    int adl_value{41};

    int& inspect() & noexcept { return member_value; }
};

int& inspect(BothRoutes& object) noexcept {
    return object.adl_value;
}

struct WrongMemberHasAdl {
    int value{50};

    int inspect(int) const noexcept { return -1; }
};

int& inspect(WrongMemberHasAdl& object) noexcept {
    return object.value;
}

struct NoRoute {
    int value{60};
};

struct ThrowingMember {
    int value{70};

    int& inspect() & {
        throw std::runtime_error("inspect failed");
    }
};

struct ThrowingAdl {
    int value{80};
};

int& inspect(ThrowingAdl& object) {
    return object.value;
}

} // namespace c04_l04_fixture

namespace c04_l04_checks {

template<class T>
concept c04_inspectable = requires(T&& object) {
    c04_lookup::inspect(std::forward<T>(object));
};

template<class T>
using inspect_result_t = decltype(c04_lookup::inspect(std::declval<T>()));

inline void check_constraints() {
    check(!c04_inspectable<c04_l04_fixture::NoRoute&>,
        "no-route object must not satisfy c04_inspectable");
}

inline void check_types() {
    check((std::same_as<inspect_result_t<c04_l04_fixture::MemberOnly&>, int&>),
        "member lvalue keeps int reference");
    check((std::same_as<inspect_result_t<const c04_l04_fixture::MemberOnly&>, const int&>),
        "const member keeps const int reference");
    check((std::same_as<inspect_result_t<c04_l04_fixture::MemberOnly&&>, int&&>),
        "rvalue member keeps rvalue reference");
    check((std::same_as<inspect_result_t<c04_l04_fixture::AdlOnly&>, int&>),
        "ADL lvalue keeps int reference");
    check((std::same_as<inspect_result_t<const c04_l04_fixture::AdlOnly&>, const int&>),
        "const ADL keeps const int reference");
    check((std::same_as<inspect_result_t<c04_l04_fixture::HiddenFriend&>, int&>),
        "hidden friend is found by ADL");
    check((std::same_as<inspect_result_t<c04_l04_fixture::WrongMemberHasAdl&>, int&>),
        "unusable member does not suppress valid ADL path");
}

template<class Object>
void check_int_reference(Object&& object, int* expected, int replacement,
                         const char* identity_message, const char* write_message) {
    if constexpr (std::same_as<inspect_result_t<Object>, int&>) {
        int& result = c04_lookup::inspect(std::forward<Object>(object));
        check(&result == expected, identity_message);
        result = replacement;
        check(*expected == replacement, write_message);
    } else {
        check(false, identity_message);
    }
}

template<class Object>
void check_member_priority(Object&& object, int* member_value, int* adl_value) {
    if constexpr (std::same_as<inspect_result_t<Object>, int&>) {
        int& result = c04_lookup::inspect(std::forward<Object>(object));
        check(&result == member_value, "member route wins over ADL route");
        check(&result != adl_value, "ADL route is not chosen when member is valid");
    } else {
        check(false, "member route wins over ADL route");
    }
}

inline void check_identity_and_priority() {
    c04_l04_fixture::MemberOnly member{};
    check_int_reference(member, &member.value, 11,
        "member route returns original subobject",
        "member route writes through");

    c04_l04_fixture::AdlOnly adl{};
    check_int_reference(adl, &adl.value, 21,
        "ADL route returns original subobject",
        "ADL route writes through");

    c04_l04_fixture::HiddenFriend hidden{};
    check_int_reference(hidden, &hidden.value, 31,
        "hidden friend route returns original subobject",
        "hidden friend route writes through");

    c04_l04_fixture::BothRoutes both{};
    check_member_priority(both, &both.member_value, &both.adl_value);

    c04_l04_fixture::WrongMemberHasAdl fallback{};
    check_int_reference(fallback, &fallback.value, 51,
        "ADL route survives unusable same-name member",
        "ADL fallback writes through");
}

inline void check_noexcept_contract() {
    c04_l04_fixture::MemberOnly member{};
    c04_l04_fixture::AdlOnly adl{};
    c04_l04_fixture::ThrowingMember throwing_member{};
    c04_l04_fixture::ThrowingAdl throwing_adl{};

    check(noexcept(c04_lookup::inspect(member)), "nothrow member propagates noexcept");
    check(noexcept(c04_lookup::inspect(adl)), "nothrow ADL propagates noexcept");
    check(!noexcept(c04_lookup::inspect(throwing_member)), "throwing member is not noexcept");
    check(!noexcept(c04_lookup::inspect(throwing_adl)), "throwing ADL is not noexcept");

    bool threw = false;
    try {
        (void)c04_lookup::inspect(throwing_member);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    check(threw, "throwing member exception propagates");
}

} // namespace c04_l04_checks

int main() {
    c04_l04_checks::check_constraints();
    c04_l04_checks::check_types();
    c04_l04_checks::check_identity_and_priority();
    c04_l04_checks::check_noexcept_contract();
    std::cout << "L04_lookup checks OK\n";
}
