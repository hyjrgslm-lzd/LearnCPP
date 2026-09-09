// OBSERVATION baseline: success covers only the checks below, not every exercise Part.
#include "reference.hpp"
#include <iostream>
int main() {
    cs::layout::counters c;
    c.run(cs::layout::variant::packed,1000);
    c.verify(cs::layout::variant::packed,1000);
    std::cout << "Observation: packed final total = 2000; this does not grade all Parts\n";
}
