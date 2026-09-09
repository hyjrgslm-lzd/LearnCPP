#include <check.hpp>

#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

namespace {

struct LifetimeProbe {
    explicit LifetimeProbe(int id) : id(id) { ++live; }
    LifetimeProbe(const LifetimeProbe& other) : id(other.id) { ++live; }
    LifetimeProbe(LifetimeProbe&& other) noexcept : id(other.id) { ++live; }
    LifetimeProbe& operator=(const LifetimeProbe&) = default;
    LifetimeProbe& operator=(LifetimeProbe&&) noexcept = default;
    ~LifetimeProbe() { --live; }

    int id = 0;
    inline static int live = 0;
};

struct ThrowingCtor {
    explicit ThrowingCtor(bool should_throw)
    {
        if (should_throw) {
            throw std::runtime_error("planned constructor failure");
        }
    }
};

std::optional<int> parse_positive_digit(char c)
{
    if (c >= '1' && c <= '9') {
        return c - '0';
    }
    return std::nullopt;
}

std::optional<int> half_if_even(int value)
{
    if (value % 2 == 0) {
        return value / 2;
    }
    return std::nullopt;
}

int default_value(int& calls)
{
    ++calls;
    return 99;
}

void check_lifetime_and_reset()
{
    check(LifetimeProbe::live == 0, "lifetime probe starts empty");
    std::optional<LifetimeProbe> probe;
    check(!probe.has_value(), "default optional is empty");
    probe.emplace(7);
    check(probe->id == 7, "emplace constructs contained object");
    check(LifetimeProbe::live == 1, "optional owns live object after emplace");
    probe.reset();
    check(!probe, "reset makes optional empty");
    check(LifetimeProbe::live == 0, "reset destroys contained object");
}

void check_emplace_failure_empties()
{
    std::optional<ThrowingCtor> value{std::in_place, false};
    check(value.has_value(), "optional starts with a value");

    bool threw = false;
    try {
        value.emplace(true);
    } catch (const std::runtime_error&) {
        threw = true;
    }

    check(threw, "emplace reports constructor failure");
    check(!value.has_value(), "failed emplace leaves optional empty");
}

void check_access_and_value_or()
{
    std::optional<int> empty;
    bool threw = false;
    try {
        (void)empty.value();
    } catch (const std::bad_optional_access&) {
        threw = true;
    }
    check(threw, "value on empty optional throws bad_optional_access");

    std::optional<int> present = 5;
    check(*present == 5, "operator star is valid after has_value check");

    int calls = 0;
    check(present.value_or(default_value(calls)) == 5, "value_or keeps present value");
    check(calls == 1, "value_or default argument is evaluated before call");
    check(empty.value_or(default_value(calls)) == 99, "value_or uses default for empty optional");
    check(calls == 2, "value_or evaluated default for empty optional too");

    std::optional<std::string> text = std::string("owned");
    std::string moved = std::move(text).value_or("fallback");
    check(moved == "owned", "rvalue optional can move contained value");
}

void check_monadic_short_circuit()
{
    int calls = 0;
    auto present = parse_positive_digit('8')
        .and_then([&](int value) {
            ++calls;
            return half_if_even(value);
        })
        .transform([&](int value) {
            ++calls;
            return value + 1;
        })
        .or_else([&]() -> std::optional<int> {
            ++calls;
            return 0;
        });
    check(present == 5, "and_then and transform produce wrapped value");
    check(calls == 2, "or_else is skipped for present optional");

    calls = 0;
    auto missing = parse_positive_digit('x')
        .and_then([&](int value) {
            ++calls;
            return half_if_even(value);
        })
        .transform([&](int value) {
            ++calls;
            return value + 1;
        })
        .or_else([&]() -> std::optional<int> {
            ++calls;
            return 42;
        });
    check(missing == 42, "or_else can recover an empty optional");
    check(calls == 1, "and_then and transform short-circuit on empty optional");

    bool threw = false;
    try {
        (void)parse_positive_digit('2').transform([](int) -> int {
            throw std::runtime_error("callback failure");
        });
    } catch (const std::runtime_error&) {
        threw = true;
    }
    check(threw, "optional monadic callbacks propagate exceptions");
}

} // namespace

int main()
{
    check_lifetime_and_reset();
    check_emplace_failure_empties();
    check_access_and_value_or();
    check_monadic_short_circuit();
}
