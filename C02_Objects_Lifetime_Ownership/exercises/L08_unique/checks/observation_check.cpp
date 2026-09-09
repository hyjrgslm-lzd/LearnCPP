#include "support/unique_support.hpp"

#include <check.hpp>

#include <iostream>
#include <memory>

int main() {
    l08_support::reset_model();
    {
        std::unique_ptr<l08_support::Tracked, l08_support::CountingDelete> one(new l08_support::Tracked(1));
        auto two = std::move(one);
        check(one == nullptr, "moved-from unique_ptr is empty");
        check(two != nullptr && two->value == 1, "moved-to unique_ptr owns object");
        auto* raw = two.release();
        check(two == nullptr, "release empties unique_ptr");
        check(l08_support::counters().alive == 1, "release does not delete");
        l08_support::CountingDelete{}(raw);
    }
    check(l08_support::counters().alive == 0, "observation leaves no live object");
    std::cout << "L08_unique_observation OK\n";
}

