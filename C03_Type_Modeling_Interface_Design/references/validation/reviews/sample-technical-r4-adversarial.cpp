#include <table.hpp>

#include <check.hpp>

#include <initializer_list>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {

std::vector<int> values_of(const l06::Table& table)
{
    std::vector<int> result;
    for (const l06::TrackedValue& value : table.view()) {
        result.push_back(value.value());
    }
    return result;
}

void check_values(const l06::Table& table, std::initializer_list<int> expected, const char* message)
{
    check(values_of(table) == std::vector<int>(expected), message);
}

void reset_copy_plan()
{
    l06::TrackedValue::reset_copy_plan();
}

void check_strong_prefix_contract_across_copy_points()
{
    for (int copy_index = 1; copy_index <= 8; ++copy_index) {
        l06::Table table{1, 2, 3};
        std::vector<l06::TrackedValue> source;
        source.emplace_back(7);
        source.emplace_back(8);

        reset_copy_plan();
        l06::TrackedValue::throw_on_copy(copy_index);

        bool threw = false;
        try {
            table.replace_prefix_strong(source);
        } catch (const std::runtime_error&) {
            threw = true;
        }

        if (threw) {
            check_values(table, {1, 2, 3}, "strong prefix keeps original when copy failure is reached");
        } else {
            check_values(table, {7, 8, 3}, "strong prefix completes when copy failure is not reached");
        }
    }
}

void check_replace_all_self_contract_across_copy_points()
{
    for (int copy_index = 1; copy_index <= 6; ++copy_index) {
        l06::Table table{4, 5};
        std::span<const l06::TrackedValue> own = table.view();

        reset_copy_plan();
        l06::TrackedValue::throw_on_copy(copy_index);

        bool threw = false;
        try {
            table.replace_all_strong(own);
        } catch (const std::runtime_error&) {
            threw = true;
        }

        if (threw) {
            check_values(table, {4, 5}, "self all-replace keeps original when copy failure is reached");
        } else {
            check_values(table, {4, 5}, "self all-replace completes when copy failure is not reached");
        }
    }
}

void check_oversize_rejection_and_reuse()
{
    l06::Table table{1, 2};
    std::vector<l06::TrackedValue> source;
    source.emplace_back(7);
    source.emplace_back(8);
    source.emplace_back(9);

    bool basic_threw = false;
    try {
        table.replace_prefix_basic(source);
    } catch (const std::out_of_range&) {
        basic_threw = true;
    }
    check(basic_threw, "basic prefix rejects oversize");
    check_values(table, {1, 2}, "basic oversize keeps original");

    bool strong_threw = false;
    try {
        table.replace_prefix_strong(source);
    } catch (const std::out_of_range&) {
        strong_threw = true;
    }
    check(strong_threw, "strong prefix rejects oversize");
    check_values(table, {1, 2}, "strong oversize keeps original");

    std::vector<l06::TrackedValue> next;
    next.emplace_back(5);
    reset_copy_plan();
    table.replace_prefix_strong(next);
    check_values(table, {5, 2}, "table remains usable after oversize rejection");
}

} // namespace

int main()
{
    static_assert(noexcept(std::declval<l06::Table&>().swap(std::declval<l06::Table&>())));

    check_strong_prefix_contract_across_copy_points();
    check_replace_all_self_contract_across_copy_points();
    check_oversize_rejection_and_reuse();
    check(l06::TrackedValue::live_count() == 0, "adversarial test releases tracked values");
}
