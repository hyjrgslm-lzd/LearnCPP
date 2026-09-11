#include "mini/stop_token.hpp"
#include "coroutine_study/exercise_check.hpp"
#include <iostream>

int main() {
    mini::stop_source source;
    int callbacks = 0;
    std::stop_callback registered{source.get_token(), [&] { ++callbacks; }};
    coroutine_study::check(!source.stop_requested(), "new source must not be stopped");
    coroutine_study::check(source.request_stop() && !source.request_stop(), "stop request must be idempotent");
    std::stop_callback late{source.get_token(), [&] { ++callbacks; }};
    coroutine_study::check(callbacks == 2, "registered and late callbacks must each run once");
    std::cout << "provided stop_token aliases: idempotence and callbacks checked\n";
}
