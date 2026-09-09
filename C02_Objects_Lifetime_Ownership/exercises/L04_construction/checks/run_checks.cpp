#include <array>
#include <cstddef>
#include <exception>
#include <string>
#include <vector>

#include "check.hpp"
#include "l04_checks.hpp"

#include L04_IMPLEMENTATION_HEADER

namespace {

void check_sequence(std::vector<std::string> const& actual,
                    std::vector<std::string> const& expected,
                    char const* label) {
    check(actual == expected, label);
}

void check_events(l04_checks::Recorder const& recorder,
                  std::initializer_list<l04_checks::Event> expected,
                  char const* label) {
    check(recorder.event_count == expected.size(), label);

    std::size_t index = 0;
    for (auto event : expected) {
        check(recorder.events[index] == event, label);
        ++index;
    }
}

void check_order() {
    check_sequence(l04::observe_order(),
                   {
                       "Base()",
                       "Member first()",
                       "Member second()",
                       "Derived body",
                       "~Derived body",
                       "~Member second()",
                       "~Member first()",
                       "~Base()",
                   },
                   "construction and destruction order");
}

void check_successful_pair() {
    l04_checks::Recorder recorder;
    {
        l04::TwoResourceOwner owner(recorder);
        check(recorder.live == 2, "both resources should be live inside the complete object scope");
        recorder.observe_body();
    }

    check(recorder.live == 0, "success path should release both resources after scope exit");
    check_events(recorder,
                 {
                     l04_checks::Event::acquire_first,
                     l04_checks::Event::acquire_second,
                     l04_checks::Event::body_observed,
                     l04_checks::Event::release_second,
                     l04_checks::Event::release_first,
                 },
                 "success path event sequence");
}

void check_second_acquire_failure() {
    l04_checks::Recorder recorder;
    recorder.fail_acquire_at = 2;

    bool caught = false;
    try {
        l04::TwoResourceOwner owner(recorder);
        recorder.observe_body();
    } catch (l04_checks::AcquireError const&) {
        caught = true;
    }

    check(caught, "second acquire should throw the model acquire failure");
    check(recorder.live == 0, "completed first member should release during unwinding");
    check_events(recorder,
                 {
                     l04_checks::Event::acquire_first,
                     l04_checks::Event::acquire_second,
                     l04_checks::Event::throw_second,
                     l04_checks::Event::release_first,
                 },
                 "failure path event sequence");
}

} // namespace

int main() {
    check_successful_pair();
    check_order();
    check_second_acquire_failure();
}
