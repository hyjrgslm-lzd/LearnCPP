#include <coroutine_study/exercise_check.hpp>

#include <generator>
#include <iostream>
#include <vector>

using coroutine_study::check;

namespace {
struct observations {
    int entered = 0;
    int yielded = 0;
    int continued = 0;
    int local_destroyed = 0;
};

struct local_lifetime {
    observations& events;
    ~local_lifetime() { ++events.local_destroyed; }
};

std::generator<int> numbers(observations& events) {
    ++events.entered;
    local_lifetime local{events};
    for (int value : {1, 2, 3}) {
        ++events.yielded;
        co_yield value;
        ++events.continued;
    }
}
} // namespace

// 对应讲义：00-预备知识-执行模型与标准库.md，练习 P-2。
int main() {
    {
        observations events;
        auto sequence = numbers(events);
        check(events.entered == 0, "constructing a generator leaves its body suspended");
        auto it = sequence.begin();
        check(*it == 1 && events.yielded == 1, "begin runs to the first yield");
        check(*it == 1 && events.yielded == 1, "dereferencing does not advance the producer");
        ++it;
        check(*it == 2 && events.continued == 1, "increment continues after the previous yield");
        ++it;
        check(*it == 3, "the third element is available");
        ++it;
        check(it == sequence.end(), "increment after the last element finishes the body");
        check(events.continued == 3 && events.local_destroyed == 1, "normal completion destroys body locals");
        std::cout << "P2/1 values=1,1,2,3 continued=3 local_destroyed=1\n";
    }
    {
        observations events;
        {
            auto sequence = numbers(events);
            auto it = sequence.begin();
            check(*it == 1 && events.local_destroyed == 0, "the body local is alive at yield");
        }
        check(events.yielded == 1 && events.continued == 0, "destroying the owner does not run the next statement");
        check(events.local_destroyed == 1, "destroying a suspended generator destroys its live locals");
        std::cout << "P2/2 yielded=1 continued=0 local_destroyed=1\n";
    }
    {
        observations events;
        auto sequence = numbers(events);
        std::vector<int> saved;
        for (int value : sequence) saved.push_back(value);
        int first_sum = 0;
        int second_sum = 0;
        for (int value : saved) first_sum += value;
        for (int value : saved) second_sum += value;
        check((saved == std::vector<int>{1, 2, 3}), "materialization saves the produced values");
        check(first_sum == 6 && second_sum == 6, "a vector supports multiple passes over saved values");
        check(events.entered == 1, "only one generator traversal produced the saved values");
        std::cout << "P2/3 first_sum=6 second_sum=6 body_entries=1\n";
    }
    std::cout << "P2_reference OK\n";
}
