#include "concurrency_study/numa.hpp"

int main() try {
    std::cout << "OBSERVATION / PLATFORM PROBE ONLY: local pages; exit 0 does not complete remote/interleaved/shared-reader Parts.\n";
    const auto topology = cs::numa::discover();
    cs::numa::describe(topology, std::cout);
    cs::numa::placement_experiment local(topology, "local", topology.page_size * 16);
    local.prepare(std::cout); local.scan(); local.verify(std::cout);
    std::cout << "Predict remote/parallel-init page maps, then run the Reference.\n";
} catch (const cs::numa::unavailable& e) { std::cerr << "SKIP: " << e.what() << '\n'; return 77; }
catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
