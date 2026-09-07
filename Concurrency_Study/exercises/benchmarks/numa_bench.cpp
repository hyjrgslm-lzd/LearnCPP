#include "concurrency_study/benchmark.hpp"
#include "concurrency_study/numa.hpp"

int main(int argc, char** argv) try {
    cs::bench::arguments args(argc, argv);
    const auto bytes = args.number("--size", 32 * 1024 * 1024);
    const auto variant = args.text("--variant", "local");
    args.finish();
    cs::check(bytes <= 1024ULL * 1024 * 1024, "size limited to 1 GiB");
    auto topology = cs::numa::discover();
    cs::numa::describe(topology, std::cerr);
    cs::numa::placement_experiment experiment(topology, variant, bytes);
    experiment.prepare(std::cerr);
    const auto ms = cs::bench::measure_ms([&] { experiment.scan(); });
    experiment.verify(std::cerr);
    cs::bench::emit_row("numa", variant, bytes, experiment.threads(), ms, experiment.completed(),
        "size=bytes; completed=uint64 words; includes reader creation binding page-pointer scan join; excludes allocation initial touch pre-read queries; " + experiment.layout());
} catch (const cs::numa::unavailable& e) { std::cerr << "SKIP: " << e.what() << '\n'; return 77; }
catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
