#include <check.hpp>
#include <g2_common/common.hpp>
#include <g2_common/heavy.hpp>

int main() {
    check(g2_common::header_heavy_value("main") > 0, "heavy public header must be parsed by main TU");
    check(g2_common::answer() == 42, "build-cost target changed behavior");
}
