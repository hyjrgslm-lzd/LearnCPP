#include <table.hpp>

#include <check.hpp>

#include <initializer_list>
#include <span>
#include <stdexcept>
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

void check_basic_prefix_failure()
{
    l06::Table table{1, 2, 3};
    std::vector<l06::TrackedValue> source;
    source.emplace_back(7);
    source.emplace_back(8);

    reset_copy_plan();
    l06::TrackedValue::throw_on_copy(2);

    bool threw = false;
    try {
        table.replace_prefix_basic(source);
    } catch (const std::runtime_error&) {
        threw = true;
    }

    check(threw, "basic prefix failure reports copy error");
    check_values(table, {7, 2, 3}, "basic failure keeps legal partial update");
    check(l06::TrackedValue::live_count() == 5, "basic failure keeps all live values accounted");
}

void check_prefix_size_reject_and_reuse()
{
    l06::Table table{1, 2};
    std::vector<l06::TrackedValue> too_many;
    too_many.emplace_back(4);
    too_many.emplace_back(5);
    too_many.emplace_back(6);

    bool threw = false;
    try {
        table.replace_prefix_basic(too_many);
    } catch (const std::out_of_range&) {
        threw = true;
    }
    check(threw, "oversized prefix is rejected");
    check_values(table, {1, 2}, "oversized prefix keeps table unchanged");

    std::vector<l06::TrackedValue> next;
    next.emplace_back(8);
    reset_copy_plan();
    table.replace_prefix_basic(next);
    check_values(table, {8, 2}, "table remains usable after rejected prefix");
}

void check_strong_prefix_same_success_contract()
{
    l06::Table basic{1, 2, 3};
    l06::Table strong{1, 2, 3};
    std::vector<l06::TrackedValue> source;
    source.emplace_back(7);
    source.emplace_back(8);

    reset_copy_plan();
    basic.replace_prefix_basic(source);
    reset_copy_plan();
    strong.replace_prefix_strong(source);

    check(values_of(basic) == values_of(strong), "strong prefix success matches basic prefix success");
}

void check_strong_prefix_failure_at(int copy_index)
{
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
        check_values(table, {1, 2, 3}, "strong prefix failure keeps original prefix");
    } else {
        check_values(table, {7, 8, 3}, "strong prefix succeeds when copy plan is not reached");
    }
}

void check_strong_prefix_failure_points()
{
    check_strong_prefix_failure_at(1);
    check_strong_prefix_failure_at(2);
    check_strong_prefix_failure_at(3);
    check_strong_prefix_failure_at(4);
    check_strong_prefix_failure_at(5);
}

void check_strong_failure_at(int copy_index)
{
    l06::Table table{1, 2, 3};
    std::vector<l06::TrackedValue> source;
    source.emplace_back(7);
    source.emplace_back(8);
    source.emplace_back(9);

    reset_copy_plan();
    l06::TrackedValue::throw_on_copy(copy_index);

    bool threw = false;
    try {
        table.replace_all_strong(source);
    } catch (const std::runtime_error&) {
        threw = true;
    }

    check(threw, "strong failure reports copy error");
    check_values(table, {1, 2, 3}, "strong failure keeps original table");
    check(l06::TrackedValue::live_count() == 6, "strong failure releases prepared prefix");
}

void check_all_prepare_failure_points()
{
    check_strong_failure_at(1);
    check_strong_failure_at(2);
    check_strong_failure_at(3);
}

void check_strong_success_empty_and_self()
{
    l06::Table table{1, 2, 3};
    std::vector<l06::TrackedValue> source;
    source.emplace_back(4);
    source.emplace_back(5);

    reset_copy_plan();
    table.replace_all_strong(source);
    check_values(table, {4, 5}, "strong success replaces all values");

    std::span<const l06::TrackedValue> own = table.view();
    reset_copy_plan();
    table.replace_all_strong(own);
    check_values(table, {4, 5}, "self view strong replace preserves values");

    reset_copy_plan();
    table.replace_all_strong({});
    check(table.view().empty(), "empty strong replace is valid");
}

} // namespace

int main()
{
    check_basic_prefix_failure();
    check_prefix_size_reject_and_reuse();
    check_strong_prefix_same_success_contract();
    check_strong_prefix_failure_points();
    check_all_prepare_failure_points();
    check_strong_success_empty_and_self();
    check(l06::TrackedValue::live_count() == 0, "all tracked values released");
}
