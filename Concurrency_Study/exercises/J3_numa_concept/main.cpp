#include "concurrency_study/numa.hpp"

int main() try {
    std::cout << "OBSERVATION / PLATFORM PROBE ONLY: topology and one CPU binding; not completion of every Part.\n";
    const auto topology = cs::numa::discover();
    cs::numa::describe(topology, std::cout);
    const auto target = topology.allowed.front();
    cs::numa::on_cpus(std::vector<cs::numa::cpu>{target}, [&](std::size_t) {
        const auto actual = cs::numa::current_cpu();
        cs::check(actual == target, "requested CPU observed");
    });
    std::cout << "One allowed CPU verified. Reference probes up to four; N1 verifies pages.\n";
} catch (const cs::numa::unavailable& e) { std::cerr << "SKIP: " << e.what() << '\n'; return 77; }
catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
