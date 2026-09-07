// OBSERVATION baseline: success covers only the checks below, not every exercise Part.
#include "../J1_false_sharing/reference.hpp"
#include <iostream>
int main() {
    using namespace cs::layout;
    std::cout << "destructive=" << destructive << " constructive=" << constructive
              << " implementation_hint=" << implementation_hint
              << " packed=" << sizeof(packed_pair) << " padded=" << sizeof(padded_counter) << '\n';
}
