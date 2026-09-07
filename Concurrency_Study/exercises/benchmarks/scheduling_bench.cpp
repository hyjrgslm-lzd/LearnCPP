#include "concurrency_study/benchmark.hpp"
#include "concurrency_study/exercise_check.hpp"
#include "concurrency_study/work_stealing_pool.hpp"
#include <numeric>

int main(int argc, char** argv) try {
    cs::bench::arguments args(argc, argv);
    const auto size = args.number("--size", 4096);
    const auto threads = args.number("--threads", 4);
    const auto variant = args.text("--variant", "static");
    args.finish();
    cs::check(size <= 10000000 && threads > 0 && threads <= 256, "size <= 10000000; threads in 1..256");
    std::vector<std::uint64_t> values;
    const auto ms = cs::bench::measure_ms([&] { values = cs::scheduling::run(variant, size, threads); });
    for (std::size_t id = 0; id < size; ++id)
        cs::check(values[id] == cs::scheduling::work(id, size), "completed ID result");
    cs::bench::emit_row("scheduling", variant, size, threads, ms, values.size(),
        "includes allocation; thread/pool creation; submit; compute; join; first quarter 100x rounds");
} catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
