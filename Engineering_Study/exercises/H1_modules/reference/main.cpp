import geometry;

#include <check.hpp>

int main() {
    check(unit_scale() == 1, "exported partition declaration should be reachable through the primary module");
    check(square_area(6) == 36, "module implementation unit changed behavior");
}
