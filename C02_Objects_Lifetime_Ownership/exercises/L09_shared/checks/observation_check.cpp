#include "support/shared_support.hpp"

#include <check.hpp>

#include <iostream>
#include <memory>

int main() {
    l09_support::reset_model();
    std::weak_ptr<l09_support::Node> weak;
    {
        auto owner = std::make_shared<l09_support::Node>(1);
        weak = owner;
        auto alias = std::shared_ptr<int>(owner, &owner->value);
        owner.reset();
        check(!weak.expired(), "alias owner keeps control block alive");
        check(*alias == 1, "alias stored pointer points to member");
    }
    check(weak.expired(), "weak expires after alias releases last strong");
    check(l09_support::counters().nodes_alive == 0, "observation leaves no nodes");
    std::cout << "L09_shared_observation OK\n";
}

