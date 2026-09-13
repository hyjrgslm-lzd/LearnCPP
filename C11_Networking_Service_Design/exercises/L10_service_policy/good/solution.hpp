#pragma once
#include <c11/service_policy.hpp>
namespace exercise {
inline std::optional<std::chrono::milliseconds> retry(const c11::retry_context& c,std::mt19937& rng) {
    using namespace std::chrono;
    if(!c.transient || !c.idempotent || c.max_attempts<1 || c.max_attempts>8 || c.attempt>=c.max_attempts-1 || c.deadline<=c.now || c.retry_after.count()<0)return {};
    unsigned maximum=25;for(unsigned i=0;i<c.attempt && maximum<500;++i)maximum=std::min(500u,maximum*2);
    const auto wait=std::max(milliseconds(std::uniform_int_distribution<unsigned>(0,maximum)(rng)),c.retry_after);
    return wait<c.deadline-c.now?std::optional<milliseconds>{wait}:std::nullopt;
}
}
