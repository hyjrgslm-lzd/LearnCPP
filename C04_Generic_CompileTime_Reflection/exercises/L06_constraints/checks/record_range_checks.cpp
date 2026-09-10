#include <check.hpp>
#include <record_range.hpp>

#include <array>
#include <concepts>
#include <cstddef>
#include <iostream>
#include <iterator>
#include <ranges>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace c04_l06_fixture {

struct Field {
    std::string_view name;
    int value;
};

struct MissingName {
    int value;
};

struct NonIntegralValue {
    std::string_view name;
    double value;
};

struct TokenStream {
    struct iterator {
        using iterator_concept = std::input_iterator_tag;
        using iterator_category = std::input_iterator_tag;
        using value_type = Field;
        using difference_type = std::ptrdiff_t;

        const Field* current{};
        const Field* last{};

        const Field& operator*() const noexcept { return *current; }
        const Field* operator->() const noexcept { return current; }

        iterator& operator++() noexcept {
            if (current != last) {
                ++current;
            }
            return *this;
        }

        void operator++(int) noexcept {
            ++(*this);
        }

        friend bool operator==(const iterator& left, std::default_sentinel_t) noexcept {
            return left.current == left.last;
        }
    };

    const Field* first{};
    const Field* last{};

    iterator begin() const noexcept { return {first, last}; }
    std::default_sentinel_t end() const noexcept { return {}; }
};

} // namespace c04_l06_fixture

namespace c04_l06_checks {

template<class R>
concept countable = requires(R&& range) {
    c04_constraints::field_count(static_cast<R&&>(range));
};

inline void check_field_like() {
    check(c04_constraints::field_like<c04_l06_fixture::Field&>,
        "field with string_view name and int value is field_like");
    check(!c04_constraints::field_like<c04_l06_fixture::MissingName&>,
        "missing name is not field_like");
    check(!c04_constraints::field_like<c04_l06_fixture::NonIntegralValue&>,
        "non-integral value is not field_like");
}

inline void check_ranges() {
    using fields_t = std::vector<c04_l06_fixture::Field>;
    check(c04_constraints::stable_field_range<fields_t&>,
        "vector of fields is a stable field range");
    check(countable<fields_t&>, "field_count accepts a stable field range");

    std::vector<c04_l06_fixture::Field> fields{{"id", 1}, {"hp", 2}, {"mp", 3}};
    check(c04_constraints::field_count(fields) == 3, "field_count counts every field");
    check(c04_constraints::field_count(fields) == 3, "forward range can be counted twice");

    std::array<c04_l06_fixture::Field, 2> array{{{"x", 4}, {"y", 5}}};
    check(c04_constraints::field_count(array) == 2, "array of fields is accepted");
}

inline void check_rejections() {
    using bad_value_t = std::vector<c04_l06_fixture::NonIntegralValue>;
    check(!c04_constraints::stable_field_range<bad_value_t&>,
        "range with non-integral values is rejected");
    check(!countable<bad_value_t&>, "field_count rejects invalid field element shape");

    c04_l06_fixture::Field storage[]{{"a", 1}, {"b", 2}};
    using single_pass_t = c04_l06_fixture::TokenStream;
    single_pass_t stream{storage, storage + 2};
    (void)stream;
    check(std::ranges::input_range<single_pass_t>, "single-pass fixture is an input range");
    check(!std::ranges::forward_range<single_pass_t>, "single-pass fixture is not a forward range");
    check(!c04_constraints::stable_field_range<single_pass_t&>,
        "single-pass range must not satisfy stable_field_range");
    check(!countable<single_pass_t&>, "field_count rejects single-pass ranges");
}

} // namespace c04_l06_checks

int main() {
    c04_l06_checks::check_field_like();
    c04_l06_checks::check_ranges();
    c04_l06_checks::check_rejections();
    std::cout << "L06_constraints checks OK\n";
}
