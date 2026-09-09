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

void check_strong_prefix_source_copy_failures()
{
    for (int copy_index : {4, 5}) {
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

        check(threw, "strong prefix reports source-copy failure");
        check_values(table, {1, 2, 3}, "strong prefix keeps original after source-copy failure");
    }
}

void check_strong_prefix_commit_does_not_copy()
{
    l06::Table table{1, 2, 3};
    std::vector<l06::TrackedValue> source;
    source.emplace_back(7);
    source.emplace_back(8);

    reset_copy_plan();
    l06::TrackedValue::throw_on_copy(6);
    table.replace_prefix_strong(source);
    check_values(table, {7, 8, 3}, "strong prefix commit uses no extra throwing copy");
}

void check_replace_all_self_failure()
{
    l06::Table table{4, 5};
    std::span<const l06::TrackedValue> own = table.view();

    reset_copy_plan();
    l06::TrackedValue::throw_on_copy(1);

    bool threw = false;
    try {
        table.replace_all_strong(own);
    } catch (const std::runtime_error&) {
        threw = true;
    }

    check(threw, "self replace reports prepare failure");
    check_values(table, {4, 5}, "self replace keeps original after prepare failure");
}

} // namespace

int main()
{
    static_assert(noexcept(std::declval<l06::Table&>().swap(std::declval<l06::Table&>())));

    check_strong_prefix_source_copy_failures();
    check_strong_prefix_commit_does_not_copy();
    check_replace_all_self_failure();
    check(l06::TrackedValue::live_count() == 0, "adversarial test releases tracked values");
}
