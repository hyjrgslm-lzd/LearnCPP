#include <check.hpp>
#include <read_value.hpp>

#include <concepts>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace c04_l11_fixture {

struct MemberBox {
    int value{11};

    int& read_value() & noexcept { return value; }
    const int& read_value() const& noexcept { return value; }
    int&& read_value() && noexcept { return std::move(value); }
};

struct AdlBox {
    int value{21};
};

int& read_value(AdlBox& box) noexcept {
    return box.value;
}

const int& read_value(const AdlBox& box) noexcept {
    return box.value;
}

struct HiddenFriendBox {
    int value{31};

    friend int& read_value(HiddenFriendBox& box) noexcept {
        return box.value;
    }
};

struct BothBox {
    int member_value{41};
    int adl_value{42};

    int& read_value() & noexcept { return member_value; }
};

int& read_value(BothBox& box) noexcept {
    return box.adl_value;
}

struct NeedsArgument {
    int value{51};

    int read_value(int) const noexcept { return value; }
};

struct NeedsArgumentButHasAdl {
    int value{56};

    int read_value(int) const noexcept { return -1; }
};

int& read_value(NeedsArgumentButHasAdl& box) noexcept {
    return box.value;
}

struct NoPath {
    int value{61};
};

struct ThrowingMember {
    int value{71};

    int& read_value() & {
        throw std::runtime_error("member failure");
    }
};

struct NoThrowAdl {
    int value{81};
};

int& read_value(NoThrowAdl& box) noexcept {
    return box.value;
}

struct ThrowingAdl {
    int value{91};
};

int& read_value(ThrowingAdl& box) {
    return box.value;
}

struct MoveOnly {
    int value{101};

    MoveOnly() = default;
    MoveOnly(const MoveOnly&) = delete;
    MoveOnly& operator=(const MoveOnly&) = delete;
};

struct MoveOnlyBox {
    MoveOnly value{};

    MoveOnly& read_value() & noexcept { return value; }
};

} // namespace c04_l11_fixture

namespace c04_l11_checks {

template<class T>
concept c04_readable = requires(T&& object) {
    c04::read_value(std::forward<T>(object));
};

template<class T>
using read_result_t = decltype(c04::read_value(std::declval<T>()));

constexpr bool type_contract_ok =
    std::same_as<read_result_t<c04_l11_fixture::MemberBox&>, int&> &&
    std::same_as<read_result_t<const c04_l11_fixture::MemberBox&>, const int&> &&
    std::same_as<read_result_t<c04_l11_fixture::MemberBox&&>, int&&> &&
    std::same_as<read_result_t<c04_l11_fixture::AdlBox&>, int&> &&
    std::same_as<read_result_t<const c04_l11_fixture::AdlBox&>, const int&> &&
    std::same_as<read_result_t<c04_l11_fixture::HiddenFriendBox&>, int&> &&
    std::same_as<read_result_t<c04_l11_fixture::NeedsArgumentButHasAdl&>, int&> &&
    std::same_as<read_result_t<c04_l11_fixture::MoveOnlyBox&>, c04_l11_fixture::MoveOnly&>;

static_assert(c04_readable<c04_l11_fixture::MemberBox&>);
static_assert(c04_readable<c04_l11_fixture::AdlBox&>);
static_assert(c04_readable<c04_l11_fixture::HiddenFriendBox&>);
static_assert(c04_readable<c04_l11_fixture::BothBox&>);
static_assert(c04_readable<c04_l11_fixture::NeedsArgumentButHasAdl&>);
static_assert(c04_readable<c04_l11_fixture::MoveOnlyBox&>);

inline void check_constraints() {
    check(!c04_readable<c04_l11_fixture::NoPath&>, "no-path object must not satisfy c04_readable");
    check(!c04_readable<c04_l11_fixture::NeedsArgument&>, "member with required argument is not a valid zero-argument route");
}

inline void check_type_contract() {
    check(type_contract_ok, "read_value must preserve cvref and reference result types");
}

template<class Object>
void check_mutable_int_reference(Object&& object, int* expected, int replacement,
                                 const char* identity_message, const char* write_message) {
    if constexpr (std::same_as<read_result_t<Object>, int&>) {
        int& result = c04::read_value(std::forward<Object>(object));
        check(&result == expected, identity_message);
        result = replacement;
        check(*expected == replacement, write_message);
    } else {
        check(false, identity_message);
    }
}

template<class Object>
void check_const_int_reference(Object&& object, const int* expected, const char* message) {
    if constexpr (std::same_as<read_result_t<Object>, const int&>) {
        const int& result = c04::read_value(std::forward<Object>(object));
        check(&result == expected, message);
    } else {
        check(false, message);
    }
}

template<class Object>
void check_move_only_reference(Object&& object, c04_l11_fixture::MoveOnly* expected) {
    if constexpr (std::same_as<read_result_t<Object>, c04_l11_fixture::MoveOnly&>) {
        auto&& result = c04::read_value(std::forward<Object>(object));
        check(&result == expected, "move-only result is returned by reference");
        result.value = 102;
        check(expected->value == 102, "move-only reference writes through original object");
    } else {
        check(false, "move-only result is returned by reference");
    }
}

template<class Object>
void check_member_priority(Object&& object, int* member_value, int* adl_value) {
    if constexpr (std::same_as<read_result_t<Object>, int&>) {
        int& result = c04::read_value(std::forward<Object>(object));
        check(&result == member_value, "member route wins when member and ADL both exist");
        check(&result != adl_value, "ADL route not chosen when member is valid");
    } else {
        check(false, "member route wins when member and ADL both exist");
    }
}

inline void check_member_cvref_and_identity() {
    c04_l11_fixture::MemberBox box{};
    check_mutable_int_reference(box, &box.value, 12,
        "member lvalue result keeps reference identity",
        "member lvalue result writes through original object");

    const c04_l11_fixture::MemberBox const_box{13};
    check_const_int_reference(const_box, &const_box.value,
        "const member result keeps const reference identity");

    using rvalue_result_t = decltype(c04::read_value(c04_l11_fixture::MemberBox{}));
    check((std::same_as<rvalue_result_t, int&&>), "rvalue member result remains rvalue reference");
}

inline void check_adl_and_hidden_friend() {
    c04_l11_fixture::AdlBox adl{};
    check_mutable_int_reference(adl, &adl.value, 23,
        "ADL result keeps reference identity",
        "ADL result writes through original object");

    const c04_l11_fixture::AdlBox const_adl{22};
    check_const_int_reference(const_adl, &const_adl.value,
        "const ADL result keeps const reference identity");

    c04_l11_fixture::HiddenFriendBox hidden{};
    check_mutable_int_reference(hidden, &hidden.value, 32,
        "hidden friend found by ADL",
        "hidden friend result writes through original object");

    c04_l11_fixture::NeedsArgumentButHasAdl fallback{};
    check_mutable_int_reference(fallback, &fallback.value, 57,
        "ADL route works when same-name member needs an argument",
        "ADL fallback writes through original object");
}

inline void check_member_beats_adl() {
    c04_l11_fixture::BothBox both{};
    check_member_priority(both, &both.member_value, &both.adl_value);
}

inline void check_noexcept_contract() {
    c04_l11_fixture::MemberBox member{};
    c04_l11_fixture::NoThrowAdl nothrow_adl{};
    c04_l11_fixture::ThrowingMember throwing_member{};
    c04_l11_fixture::ThrowingAdl throwing_adl{};
    c04_l11_fixture::NeedsArgumentButHasAdl fallback{};

    check(noexcept(c04::read_value(member)), "nothrow member propagates noexcept");
    check(noexcept(c04::read_value(nothrow_adl)), "nothrow ADL propagates noexcept");
    check(noexcept(c04::read_value(fallback)), "ADL fallback after unusable member propagates noexcept");
    check(!noexcept(c04::read_value(throwing_member)), "throwing member is not declared noexcept");
    check(!noexcept(c04::read_value(throwing_adl)), "throwing ADL is not declared noexcept");

    bool threw = false;
    try {
        (void)c04::read_value(throwing_member);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    check(threw, "throwing member exception propagates");
}

inline void check_move_only_reference() {
    c04_l11_fixture::MoveOnlyBox box{};
    check_move_only_reference(box, &box.value);
}

inline void run_all() {
    check_constraints();
    check_type_contract();
    check_member_cvref_and_identity();
    check_adl_and_hidden_friend();
    check_member_beats_adl();
    check_noexcept_contract();
    check_move_only_reference();
}

} // namespace c04_l11_checks

int main() {
    c04_l11_checks::run_all();
    std::cout << "L11_customization read_value checks OK\n";
}
