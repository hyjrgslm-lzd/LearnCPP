#pragma once
#include <c11/service_policy.hpp>
namespace exercise {inline auto retry(const c11::retry_context& c,std::mt19937& random){return c11::retry_delay(c,random);}}
