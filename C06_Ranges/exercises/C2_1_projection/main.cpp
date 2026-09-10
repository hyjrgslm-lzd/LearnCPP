#include <algorithm>
#include <check.hpp>
#include <iostream>
#include <ranges>
#include <string>
#include <vector>

struct Person {
    std::string name;
    int age;
};

int main() {
    std::vector<Person> people{{"Lin", 32}, {"Ada", 28}, {"Bjarne", 74}, {"Grace", 32}};

    std::ranges::sort(people, std::less{}, &Person::age);
    check(people.front().name == "Ada", "projection sort uses age as the comparison key");

    auto lin = std::ranges::find(people, "Lin", &Person::name);
    check(lin != people.end() && lin->age == 32, "find compares the projected name");

    const Person& oldest = std::ranges::max(people, std::less{}, &Person::age);
    check(oldest.name == "Bjarne", "max returns the source element, not the projected key");

    auto [youngest_it, oldest_it] = std::ranges::minmax_element(people, std::less{}, &Person::age);
    check(youngest_it->name == "Ada", "minmax_result exposes min iterator");
    check(oldest_it->name == "Bjarne", "minmax_result exposes max iterator");
    static_assert(requires(std::ranges::minmax_result<int> r) {
        r.min;
        r.max;
    });

    auto age_32 = std::ranges::count(people, 32, &Person::age);
    check(age_32 == 2, "count with projection checks projected values");

    std::cout << "C2_1 projection checks passed\n";
}
