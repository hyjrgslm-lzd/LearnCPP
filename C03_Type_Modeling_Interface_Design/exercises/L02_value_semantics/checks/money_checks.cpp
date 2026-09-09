#include <money.hpp>

#include <check.hpp>

#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

int main()
{
    l02::Money first{1200, "CNY"};
    l02::Money copy = first;
    check(copy == first, "copy has same value");

    copy = copy.plus(l02::Money{300, "CNY"});
    check(copy.cents() == 1500, "plus returns summed value");
    check(first.cents() == 1200, "source remains unchanged after plus");

    bool threw = false;
    try {
        (void)first.plus(l02::Money{1, "USD"});
    } catch (const std::exception&) {
        threw = true;
    }
    check(threw, "different currencies cannot be added");
    check(first.cents() == 1200 && first.currency() == "CNY", "failed plus keeps input value");

    l02::Money cny{100, "CNY"};
    l02::Money usd{100, "USD"};
    check(!(cny == usd), "equality includes currency");
    check(cny.hash_key() != usd.hash_key(), "hash includes currency when equality does");

    std::vector<l02::Money> values{usd, first, cny};
    std::sort(values.begin(), values.end());
    check(values[0] == cny, "ordering groups currency before cents");
    check(values[1] == first, "ordering compares cents inside currency");
    check(values[2] == usd, "ordering keeps different currency distinct");
}
