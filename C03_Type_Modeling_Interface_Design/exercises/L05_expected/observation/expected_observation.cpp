#include <check.hpp>

#include <expected>
#include <memory>
#include <functional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace {

enum class ParseError {
    empty,
    invalid_digit,
    out_of_range,
};

enum class AppError {
    bad_input,
    duplicate_id,
};

std::expected<int, ParseError> parse_digit(std::string_view text)
{
    if (text.empty()) {
        return std::unexpected(ParseError::empty);
    }
    if (text.size() != 1 || text[0] < '0' || text[0] > '9') {
        return std::unexpected(ParseError::invalid_digit);
    }
    return text[0] - '0';
}

std::expected<void, AppError> reject_duplicate(bool duplicate)
{
    if (duplicate) {
        return std::unexpected(AppError::duplicate_id);
    }
    return {};
}

std::expected<std::reference_wrapper<int>, AppError> find_slot(bool exists, int& slot)
{
    if (!exists) {
        return std::unexpected(AppError::bad_input);
    }
    return std::ref(slot);
}

std::expected<std::reference_wrapper<const int>, AppError> find_readonly_slot(bool exists, const int& slot)
{
    if (!exists) {
        return std::unexpected(AppError::bad_input);
    }
    return std::cref(slot);
}

std::expected<int, ParseError> require_even(int value)
{
    if (value % 2 != 0) {
        return std::unexpected(ParseError::out_of_range);
    }
    return value;
}

int fallback_value(int& calls)
{
    ++calls;
    return 99;
}

ParseError fallback_error(int& calls)
{
    ++calls;
    return ParseError::empty;
}

void check_value_error_and_void()
{
    auto parsed = parse_digit("7");
    check(parsed.has_value(), "expected stores success value");
    check(*parsed == 7, "operator star reads success value");

    auto failed = parse_digit("x");
    check(!failed, "expected stores error state");
    check(failed.error() == ParseError::invalid_digit, "unexpected stores classified error");

    auto ok = reject_duplicate(false);
    check(ok.has_value(), "expected void can report success");
    auto duplicate = reject_duplicate(true);
    check(!duplicate && duplicate.error() == AppError::duplicate_id,
        "expected void keeps error payload");
}

void check_access_and_defaults()
{
    auto failed = parse_digit("");
    bool threw = false;
    try {
        (void)failed.value();
    } catch (const std::bad_expected_access<ParseError>& error) {
        threw = error.error() == ParseError::empty;
    }
    check(threw, "value on error expected throws bad_expected_access with error");

    auto parsed = parse_digit("5");
    int value_calls = 0;
    int error_calls = 0;
    check(parsed.value_or(fallback_value(value_calls)) == 5,
        "value_or keeps success value");
    check(value_calls == 1, "value_or default argument is evaluated before call");
    check(parsed.error_or(fallback_error(error_calls)) == ParseError::empty,
        "error_or returns default error for success state");
    check(error_calls == 1, "error_or default argument is evaluated before call");

    check(failed.value_or(fallback_value(value_calls)) == 99,
        "value_or returns default for error state");
    check(failed.error_or(fallback_error(error_calls)) == ParseError::empty,
        "error_or returns stored error for error state");
}

void check_monadic_and_mapping()
{
    int calls = 0;
    auto result = parse_digit("8")
        .and_then([&](int value) {
            ++calls;
            return require_even(value);
        })
        .transform([&](int value) {
            ++calls;
            return value / 2;
        })
        .or_else([&](ParseError) -> std::expected<int, ParseError> {
            ++calls;
            return 0;
        });
    check(result == 4, "expected success chain transforms value");
    check(calls == 2, "or_else is skipped on success");

    calls = 0;
    auto mapped = parse_digit("x")
        .and_then([&](int value) {
            ++calls;
            return require_even(value);
        })
        .transform_error([&](ParseError error) {
            ++calls;
            return error == ParseError::invalid_digit ? AppError::bad_input : AppError::duplicate_id;
        });
    check(!mapped && mapped.error() == AppError::bad_input,
        "transform_error maps classified error to application layer");
    check(calls == 1, "and_then short-circuits on error");

    bool threw = false;
    try {
        (void)parse_digit("2").transform([](int) -> int {
            throw std::runtime_error("callback failure");
        });
    } catch (const std::runtime_error&) {
        threw = true;
    }
    check(threw, "expected monadic callbacks propagate exceptions");
}

void check_move_only_success_value()
{
    using Owned = std::expected<std::unique_ptr<int>, ParseError>;
    static_assert(!std::is_copy_constructible_v<Owned>);
    static_assert(std::is_move_constructible_v<Owned>);

    Owned owned = std::make_unique<int>(11);
    auto consumed = std::move(owned).transform([](std::unique_ptr<int> pointer) {
        return *pointer + 1;
    });
    check(consumed == 12, "rvalue expected can move a move-only success value");
}

void check_reference_wrapper_borrow()
{
    static_assert(std::is_same_v<decltype(std::declval<const std::reference_wrapper<int>&>().get()), int&>,
        "const reference_wrapper does not make the pointee const");
    static_assert(std::is_same_v<decltype(std::declval<std::reference_wrapper<const int>>().get()), const int&>,
        "reference_wrapper<const T> makes the pointee read-only");

    int slot = 3;
    auto borrowed = find_slot(true, slot);
    check(borrowed.has_value(), "reference_wrapper can carry a borrowed success value");
    borrowed->get() = 9;
    check(slot == 9, "borrowed reference_wrapper modifies the original object");

    const auto locked_wrapper = std::ref(slot);
    locked_wrapper.get() = 10;
    check(slot == 10, "const reference_wrapper still exposes mutable pointee");

    auto readonly = find_readonly_slot(true, slot);
    check(readonly->get() == 10, "reference_wrapper<const T> reads borrowed object");

    auto missing = find_slot(false, slot);
    check(!missing && missing.error() == AppError::bad_input,
        "borrowed expected still carries an error state");
}

} // namespace

int main()
{
    check_value_error_and_void();
    check_access_and_defaults();
    check_monadic_and_mapping();
    check_move_only_success_value();
    check_reference_wrapper_borrow();
}
