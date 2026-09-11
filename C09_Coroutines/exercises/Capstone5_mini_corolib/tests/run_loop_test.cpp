#include "mini/single_thread_executor.hpp"
#include "mini/task.hpp"
#include "coroutine_study/exercise_check.hpp"
#include <iostream>

mini::task<int> scheduled(mini::single_thread_executor& loop, int& resumes) {
    co_await loop.schedule();
    ++resumes;
    co_return 23;
}
int main() {
    try {
        mini::single_thread_executor loop;
        int resumes = 0;
        auto root = scheduled(loop, resumes);
        root.h_.resume();
        if (root.h_.done()) (void)root.await_resume();
        coroutine_study::check(resumes == 0 && !root.h_.done(), "schedule must queue, not resume inline");
        coroutine_study::check(loop.run_one(), "schedule did not enqueue work");
        coroutine_study::check(root.h_.done() && root.await_resume() == 23 && resumes == 1, "queued continuation result mismatch");
        coroutine_study::check(!loop.run_one() && resumes == 1, "continuation enqueued twice");
        loop.stop();
        loop.run();
        std::cout << "run_loop: deferred continuation, one resume, empty stop checked\n";
    } catch (const std::exception& e) { std::cerr << "starter check failed: " << e.what() << '\n'; return 1; }
}
