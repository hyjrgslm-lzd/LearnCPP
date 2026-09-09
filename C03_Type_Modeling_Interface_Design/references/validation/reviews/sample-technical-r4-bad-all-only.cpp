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

} // namespace

int main()
{
    l06::Table table{1, 2, 3};
    std::vector<l06::TrackedValue> source;
    source.emplace_back(7);
    source.emplace_back(8);
    source.emplace_back(9);

    l06::TrackedValue::reset_copy_plan();
    l06::TrackedValue::throw_on_copy(2);

    bool threw = false;
    try {
        table.replace_all_strong(source);
    } catch (const std::runtime_error&) {
        threw = true;
    }

    check(threw, "all-only replace reports copy error");
    check_values(table, {1, 2, 3}, "all-only strong failure keeps original table");
    check(l06::TrackedValue::live_count() == 6, "all-only failure releases prepared values");
}
