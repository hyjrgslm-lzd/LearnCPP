#include "concurrency_study/benchmark.hpp"
#include "concurrency_study/numa.hpp"
#include <iostream>

int main(int argc, char** argv) try {
    cs::bench::arguments args(argc, argv);
    const auto run_placement = args.number("--placement", 0);
    const auto pages = args.number("--pages", 4);
    args.finish();
    cs::check(run_placement <= 1, "placement flag must be 0 or 1");
    cs::check(pages > 0 && pages <= 64, "page count cap");

    const auto topology = cs::numa::discover();
    cs::numa::describe(topology, std::cerr);
    const auto cpu = cs::numa::current_cpu();
    cs::bench::emit_row("numa_topology", "discover", topology.allowed.size(), 1, 0.0,
        topology.memory_nodes.size(), "current_cpu=" + std::to_string(cpu.group) + ':' + std::to_string(cpu.logical)
            + "; node=" + std::to_string(cpu.node) + "; timing not measured");

    if (run_placement == 0) return 0;

    cs::numa::placement_experiment experiment(topology, "firsttouch", topology.page_size * pages);
    experiment.prepare(std::cerr);
    const double ms = cs::bench::measure_ms([&] { experiment.scan(); });
    experiment.verify(std::cerr);
    cs::bench::emit_row("numa_pages", "firsttouch", topology.page_size * pages,
        experiment.threads(), ms, experiment.completed(),
        "small explicit observation; includes reader creation/binding/scan/join; no remote comparison");
    return 0;
} catch (const cs::numa::unavailable& e) {
    std::cerr << "SKIP: " << e.what() << '\n';
    return 77;
} catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
}
