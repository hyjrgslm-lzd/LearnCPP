#include "concurrency_study/benchmark.hpp"
#include "concurrency_study/exercise_check.hpp"
#include "concurrency_study/log.hpp"

#include <sstream>
#include <string>
#include <thread>
#include <vector>

int main() {
    bool caught = false;
    try { cs::check(false, "release check remains active"); }
    catch (const std::runtime_error&) { caught = true; }
    cs::check(caught, "check must not disappear under NDEBUG");
    cs::check(cs::now_ms() >= 0, "first-use timestamp must not be negative");

    char program[] = "test";
    char size_option[] = "--size";
    char count[] = "17";
    char* argv[] = {program, size_option, count};
    cs::bench::arguments arguments(3, argv);
    cs::check(arguments.number("--size", 0) == 17, "parse explicit size");
    cs::check(arguments.number("--threads", 4) == 4, "use absent default");
    arguments.finish();
    char negative[] = "-1";
    char* bad_argv[] = {program, size_option, negative};
    caught = false;
    try { cs::bench::arguments bad(3, bad_argv); (void)bad.number("--size", 0); }
    catch (const std::invalid_argument&) { caught = true; }
    cs::check(caught, "reject negative unsigned argument");
    cs::check(cs::bench::csv("a,\"b\"") == "\"a,\"\"b\"\"\"", "escape CSV quotes");

    std::ostringstream captured;
    struct restore_output {
        std::streambuf* previous;
        ~restore_output() { std::cout.rdbuf(previous); }
    } restore{std::cout.rdbuf(captured.rdbuf())};
    {
        std::vector<std::jthread> threads;
        threads.reserve(4);
        for (int worker = 0; worker < 4; ++worker) {
            threads.emplace_back([worker] {
                for (int line = 0; line < 20; ++line)
                    cs::logf("worker=", worker, ",line=", line);
            });
        }
    }
    const auto text = captured.str();
    std::size_t lines = 0;
    for (const char c : text) if (c == '\n') ++lines;
    cs::check(lines == 80, "all cooperating logs produce intact lines");
    for (int worker = 0; worker < 4; ++worker) {
        for (int line = 0; line < 20; ++line) {
            const auto suffix = "worker=" + std::to_string(worker) + ",line=" + std::to_string(line) + "\n";
            const auto position = text.find(suffix);
            cs::check(position != std::string::npos, "each log payload is present");
            cs::check(text.find(suffix, position + 1) == std::string::npos, "each payload occurs once");
        }
    }
}
