#pragma once

#include "l04_checks.hpp"

namespace l04 {

std::vector<std::string> observe_order();
l04_checks::ResourceAttempt run_two_resource_case(int fail_acquire_at);

} // namespace l04
