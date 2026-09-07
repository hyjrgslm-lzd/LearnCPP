#include "concurrency_study/numa.hpp"

enum class part_status { passed, skipped, failed };
struct part_summary {
    unsigned passed = 0, skipped = 0, failed = 0;
    void add(part_status status) {
        if (status == part_status::passed) ++passed;
        else if (status == part_status::skipped) ++skipped;
        else ++failed;
    }
    int exit_code() const { return failed ? 1 : skipped ? 77 : 0; }
};

static std::vector<cs::numa::cpu> shared_readers(const cs::numa::topology& topology) {
    std::vector<cs::numa::cpu> readers;
    for (auto c : topology.allowed) {
        if (std::find(topology.memory_nodes.begin(), topology.memory_nodes.end(), c.node) == topology.memory_nodes.end()) continue;
        if (std::none_of(readers.begin(), readers.end(), [&](auto previous) { return previous.node == c.node; }))
            readers.push_back(c);
        if (readers.size() == 2) break;
    }
    return readers;
}
static part_status shared_status(const std::vector<cs::numa::cpu>& readers) {
    return readers.size() >= 2 ? part_status::passed : part_status::skipped;
}

static void model_checks() {
    using cs::numa::interleave_phase;
    cs::check(interleave_phase({2, 7, 2, 7}, {2, 7}) == 0, "even VMA phase");
    cs::check(interleave_phase({7, 2, 7, 2}, {2, 7}) == 1, "odd VMA phase");
    cs::check(interleave_phase({7, 2, 7, 2}, {7, 2}) == 1, "reverse request order still uses ascending nodemask and odd phase");
    cs::check(interleave_phase({4, 7, 2, 4, 7, 2}, {7, 4, 2}) == 1, "three-node ordering and nonzero phase");
    cs::check(!interleave_phase({2, 2, 7, 7}, {7, 2}), "same histogram is not an interleave proof");
    cs::check(!interleave_phase({7, 2, -1, 2}, {7, 2}), "nonresident page fails cyclic validation");
    cs::check(!interleave_phase({7}, {7, 2}), "less than one full cycle cannot prove interleaving");
    cs::numa::topology synthetic;
    synthetic.memory_nodes = {0, 1};
    synthetic.allowed = {{0, 0, 0, 0, 0}};
    part_summary mixed;
    mixed.add(part_status::passed); // local
    mixed.add(shared_status(shared_readers(synthetic))); // memory nodes != CPU nodes
    mixed.add(part_status::passed); // simulated remote/interleave success cannot erase SKIP
    cs::check(mixed.exit_code() == 77 && mixed.skipped == 1, "two memory nodes and one reader must remain partial SKIP");
    mixed.add(part_status::failed);
    mixed.add(part_status::passed);
    cs::check(mixed.exit_code() == 1, "failure dominates partial SKIP and later success");
    synthetic.allowed.push_back({0, 1, 1, 0, 1});
    part_summary complete;
    complete.add(shared_status(shared_readers(synthetic)));
    cs::check(complete.exit_code() == 0, "two eligible CPU nodes satisfy reader capability model");
}

static part_status shared_read(const cs::numa::topology& topology) {
    const auto readers = shared_readers(topology);
    if (readers.empty()) throw cs::numa::unavailable("shared read needs an eligible CPU");
    const auto home = readers.front();
    cs::numa::pages memory(topology.page_size * 32, topology.page_size, {home.node});
    std::cout << "layout=" << memory.layout() << '\n';
    cs::numa::describe_pages("shared-before-touch", memory.nodes(), std::cout);
    cs::numa::on_cpus(std::vector<cs::numa::cpu>{home}, [&](std::size_t) { memory.initialize(0, memory.count()); });
    const std::vector<int> expected(memory.count(), home.node);
    auto before_pages = memory.nodes();
    cs::numa::describe_pages("shared-before-read", before_pages, std::cout);
    cs::numa::require_placement(before_pages, expected);
    std::vector<std::uint64_t> sums(readers.size());
    std::vector<cs::numa::cpu> before(readers.size()), after(readers.size());
    cs::numa::on_cpus(readers, [&](std::size_t i) {
        before[i] = cs::numa::current_cpu();
        sums[i] = memory.sum(0, memory.count()); // Same immutable pages, multiple readers.
        after[i] = cs::numa::current_cpu();
    });
    auto after_pages = memory.nodes();
    cs::numa::describe_pages("shared-after-read", after_pages, std::cout);
    cs::numa::require_placement(after_pages, expected);
    for (std::size_t i = 0; i < readers.size(); ++i) {
        cs::check(sums[i] == memory.words(), "each reader independently consumes the shared immutable pages");
        std::cout << "shared-reader=" << i << " actual-before=" << before[i].group << ':' << before[i].logical
            << " node=" << before[i].node << " actual-after=" << after[i].group << ':' << after[i].logical
            << " node=" << after[i].node << '\n';
    }
    std::cout << (readers.size() == 2 ? "cross-node shared read verified\n" : "shared read local check passed; cross-node readers unavailable\n");
    return shared_status(readers);
}

int main() try {
    model_checks();
    std::cout << "pure models PASS: interleave phase/order; per-Part result merge (no simulated hardware measurements)\n";
    auto topology = cs::numa::discover();
    cs::numa::describe(topology, std::cout);
    part_summary summary;
    auto run_part = [&](std::string_view name, auto&& operation) {
        part_status status;
        try { status = operation(); }
        catch (const cs::numa::unavailable& e) { std::cerr << name << " SKIP: " << e.what() << '\n'; status = part_status::skipped; }
        catch (const std::exception& e) { std::cerr << name << " FAIL: " << e.what() << '\n'; status = part_status::failed; }
        summary.add(status);
        std::cout << "Part " << name << ": " << (status == part_status::passed ? "PASS" : status == part_status::skipped ? "SKIP" : "FAIL") << '\n';
    };
    run_part("shared-read", [&] { return shared_read(topology); });
    for (auto variant : {"local", "firsttouch", "parallel-init", "remote", "interleaved"})
        run_part(variant, [&] {
            cs::numa::placement_experiment experiment(topology, variant, topology.page_size * 128);
            experiment.prepare(std::cout); experiment.scan(); experiment.verify(std::cout);
            return part_status::passed;
        });
    std::cout << "N1 summary: passed=" << summary.passed << " skipped=" << summary.skipped
        << " failed=" << summary.failed << " exit=" << summary.exit_code() << '\n';
    return summary.exit_code();
} catch (const cs::numa::unavailable& e) { std::cerr << "SKIP: " << e.what() << '\n'; return 77; }
catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
