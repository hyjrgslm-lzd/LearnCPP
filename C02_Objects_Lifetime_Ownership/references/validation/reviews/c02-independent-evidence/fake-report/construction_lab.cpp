#include "construction_lab.hpp"

namespace l04 {

std::vector<std::string> observe_order() {
    return {
        "Base()",
        "Member first()",
        "Member second()",
        "Derived body",
        "~Derived body",
        "~Member second()",
        "~Member first()",
        "~Base()",
    };
}

l04_checks::ResourceAttempt run_two_resource_case(int fail_acquire_at) {
    l04_checks::ResourceAttempt result;
    result.implemented = true;
    if (fail_acquire_at == 2) {
        result.error = "acquire failed";
        result.live_after = 0;
        result.events = {
            "acquire first",
            "acquire second",
            "throw second",
            "release first",
        };
        return result;
    }

    result.constructed = true;
    result.live_inside = 2;
    result.live_after = 0;
    result.events = {
        "acquire first",
        "acquire second",
        "body observed",
        "release second",
        "release first",
    };
    return result;
}

} // namespace l04
