#include "support/rc_support.hpp"

#include <check.hpp>

#include <iostream>
#include <memory>

int main() {
    l10_support::reset_model();
    std::weak_ptr<l10_support::Tracked> weak;
    {
        auto owner = std::make_shared<l10_support::Tracked>(1);
        weak = owner;
        owner.reset();
        check(l10_support::counters().objects_alive == 0, "std shared destroys object at strong zero");
        check(weak.expired(), "std weak expires");
    }
    check(l10_support::counters().objects_alive == 0, "observation leaves no object");
    std::cout << "L10_control_block_observation OK\n";
}

