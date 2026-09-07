#include "concurrency_study/numa.hpp"

int main() try {
    cs::check(cs::numa::parse_list("0-2,4") == std::set<int>({0,1,2,4}), "node list parser");
    auto topology = cs::numa::discover();
    cs::numa::describe(topology, std::cout);
    auto cpus = topology.allowed;
    if (cpus.size() > 4) cpus.resize(4);
    const auto caller = std::this_thread::get_id();
    std::vector<cs::numa::cpu> before(cpus.size()), after(cpus.size());
    cs::numa::on_cpus(cpus, [&](std::size_t i) {
        cs::check(std::this_thread::get_id() != caller, "affinity operation runs only on a dedicated new thread");
        before[i] = cs::numa::current_cpu(); after[i] = cs::numa::current_cpu();
    });
    bool propagated = false;
    try {
        cs::numa::on_cpus(std::vector<cs::numa::cpu>{cpus.front()}, [&](std::size_t) {
            cs::check(std::this_thread::get_id() != caller, "exception path also runs off caller thread");
            throw std::runtime_error("dedicated-worker-error");
        });
    } catch (const std::runtime_error& e) { propagated = std::string_view(e.what()) == "dedicated-worker-error"; }
    cs::check(propagated, "dedicated worker exception reaches caller after joining");
    for (std::size_t i = 0; i < cpus.size(); ++i) {
        cs::check(before[i] == cpus[i] && after[i] == cpus[i], "actual CPU at both observation points");
        std::cout << "requested=" << cpus[i].group << ':' << cpus[i].logical
            << " actual-before=" << before[i].group << ':' << before[i].logical
            << " actual-after=" << after[i].group << ':' << after[i].logical << " node=" << after[i].node << '\n';
    }
    std::cout << "J3 topology/affinity OK; page placement is checked separately in N1\n";
} catch (const cs::numa::unavailable& e) { std::cerr << "SKIP: " << e.what() << '\n'; return 77; }
catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
