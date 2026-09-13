#pragma once
#include <c11/service_policy.hpp>
namespace exercise {
inline std::optional<std::chrono::milliseconds> retry(const c11::retry_context&,std::mt19937&){throw std::logic_error("UNFINISHED: retry budget and full jitter");}
}
