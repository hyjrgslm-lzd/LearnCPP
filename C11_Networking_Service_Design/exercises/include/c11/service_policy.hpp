#pragma once
#include <chrono>
#include <cstddef>
#include <optional>
#include <random>
#include <stdexcept>
#include <algorithm>
namespace c11 {
using policy_clock=std::chrono::steady_clock;
struct retry_context {
    bool idempotent=false,transient=false;
    unsigned attempt=0,max_attempts=3;
    policy_clock::time_point now,deadline;
    std::chrono::milliseconds retry_after{0};
};
inline std::optional<std::chrono::milliseconds> retry_delay(const retry_context& c,std::mt19937& random) {
    using namespace std::chrono;
    if(!c.idempotent || !c.transient || !c.max_attempts || c.max_attempts>8 || c.attempt>=c.max_attempts-1 || c.now>=c.deadline || c.retry_after.count()<0)return {};
    const auto bound=std::min(500u,25u*(1u<<std::min(c.attempt,5u)));
    const auto delay=std::max(c.retry_after,milliseconds(std::uniform_int_distribution<unsigned>(0,bound)(random)));
    if(delay>=c.deadline-c.now)return {};
    return delay;
}
class token_bucket {
    std::chrono::nanoseconds period_,maximum_,credit_;
    policy_clock::time_point last_;
public:
    token_bucket(unsigned capacity,std::chrono::nanoseconds period,policy_clock::time_point now)
        :period_(period),last_(now) {
        if(!capacity || capacity>1024 || period.count()<=0 || period>std::chrono::seconds(60))throw std::invalid_argument("invalid token bucket bounds");
        maximum_=credit_=period*capacity;
    }
    bool take(policy_clock::time_point now,unsigned cost=1) {
        if(now>last_) {
            const auto cap=std::chrono::duration_cast<policy_clock::duration>(maximum_);
            const auto elapsed=(last_<=policy_clock::time_point::max()-cap && now>=last_+cap)
                ?maximum_:std::chrono::duration_cast<std::chrono::nanoseconds>(now-last_);
            credit_+=std::min(maximum_-credit_,elapsed);last_=now;
        }
        if(!cost || cost>1024 || cost>static_cast<unsigned>(maximum_/period_))return false;
        const auto required=period_*cost;if(credit_<required)return false;credit_-=required;return true;
    }
};
}
