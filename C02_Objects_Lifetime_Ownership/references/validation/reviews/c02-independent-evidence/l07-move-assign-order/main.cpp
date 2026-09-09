#include <iostream>
#include <utility>

#include <owner.hpp>

int main() {
    l07::reset_resource_model({});
    l07::two_resource_owner source;
    l07::two_resource_owner target;
    target = std::move(source);

    for (const auto& event : l07::resource_events()) {
        std::cout << event << '\n';
    }
}
