#include "concurrency_study/exercise_check.hpp"
#include "concurrency_study/work_stealing_pool.hpp"
#include <iostream>

int main() try {
    std::cout << "OBSERVATION: checked baseline for three schedulers; not completion of every exercise Part.\n";
    for (auto variant : {"static", "dynamic", "stealing"}) {
        auto values = cs::scheduling::run(variant, 32, 4);
        for (std::size_t id = 0; id < values.size(); ++id)
            cs::check(values[id] == cs::scheduling::work(id, values.size()), "baseline task result");
        std::cout << variant << ": completed " << values.size() << '\n';
    }
    std::cout << "Predict shutdown and recursive-wait behavior; implement the README tasks and run the separate Reference.\n";
} catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
