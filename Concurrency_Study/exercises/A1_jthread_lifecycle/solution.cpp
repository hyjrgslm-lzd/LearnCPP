#include "concurrency_study/exercise_check.hpp"
#include <exception>
#include <functional>
#include <iostream>
#include <memory>
#include <stop_token>
#include <system_error>
#include <thread>
#include <utility>

void part1_thread_join() {
    int result = 0;
    std::exception_ptr error;
    std::thread worker([&] {
        try { result = 21 * 2; cs::check(result == 42, "worker calculation"); }
        catch (...) { error = std::current_exception(); }
    });
    // No potentially throwing user operation between construction and join.
    worker.join();
    if (error) std::rethrow_exception(error);
    cs::check(result == 42 && !worker.joinable(), "join publishes result and clears association");
    bool rejected = false;
    try { worker.join(); }
    catch (const std::system_error&) { rejected = true; }
    cs::check(rejected, "second explicit join is an error");
    std::cout << "Part 1: worker result=42 -> join returned; second join rejected\n";
}

void part2_scope_and_move() {
    int result = 0;
    std::exception_ptr error;
    {
        std::jthread original([&] {
            try { result = 7; cs::check(result > 0, "finite worker"); }
            catch (...) { error = std::current_exception(); }
        });
        std::jthread owner(std::move(original));
        cs::check(!original.joinable() && owner.joinable(), "thread association moved");
    }
    if (error) std::rethrow_exception(error);
    cs::check(result == 7, "scope exit joins owner");
    std::cout << "Part 2: association moved; scope ended after result=7\n";
}

void part3_automatic_stop() {
    bool observed = false;
    std::stop_token saved;
    {
        std::jthread worker([&](std::stop_token token) {
            while (!token.stop_requested()) std::this_thread::yield();
            observed = true;
        });
        saved = worker.get_stop_token();
    } // Requests stop, then joins. Nothing on main waits for worker before this.
    cs::check(saved.stop_requested() && observed, "destruction requests stop before joining");
    std::cout << "Part 3: destructor requests stop -> worker observes -> scope returns\n";
}

void part4_arguments() {
    int original = 10;
    int copied_result = 0;
    std::exception_ptr error;
    {
        std::jthread worker([&](int copy) {
            try { copied_result = ++copy; cs::check(copy == 11, "copied argument"); }
            catch (...) { error = std::current_exception(); }
        }, original);
    }
    if (error) std::rethrow_exception(error);
    cs::check(original == 10 && copied_result == 11, "decayed argument is independent");
    {
        std::jthread worker([](int& value) { ++value; }, std::ref(original));
    }
    cs::check(original == 11, "reference argument requires referent to outlive join");
    auto pointer = std::make_unique<int>(42);
    int moved_result = 0;
    {
        std::jthread worker([p = std::move(pointer), &moved_result] { moved_result = *p; });
    }
    cs::check(!pointer && moved_result == 42, "closure owns moved argument");
    std::cout << "Part 4: copy result=11/original=10; ref original=11; move result=42\n";
}

int main() {
    part1_thread_join();
    part2_scope_and_move();
    part3_automatic_stop();
    part4_arguments();
    std::cout << "A1_reference OK\n";
}
