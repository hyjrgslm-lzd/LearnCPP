#include <solution.hpp>
#include <check.hpp>
#include <set>
#include <iostream>
using namespace std::chrono_literals;
int main(){try{
    std::mt19937 random(42);const c11::policy_clock::time_point now{};
    c11::retry_context context{false,true,0,3,now,now+1s};
    check(!exercise::retry(context,random),"non-idempotent outcomes are not retried");
    context.idempotent=true;context.transient=false;check(!exercise::retry(context,random),"permanent failures are not retried");
    context.transient=true;std::set<long long> samples;
    for(int i=0;i<40;++i){auto wait=exercise::retry(context,random);check(wait && *wait>=0ms && *wait<=25ms,"first retry full-jitter bounds");samples.insert(wait->count());}
    check(samples.size()>1,"jitter does not collapse into a constant delay");
    context.attempt=2;check(!exercise::retry(context,random),"total attempt limit includes initial call");
    context.attempt=1;context.retry_after=500ms;context.deadline=now+100ms;
    check(!exercise::retry(context,random),"server retry-after cannot exceed remaining total deadline");
    context.deadline=now+1s;check(exercise::retry(context,random)==500ms,"server retry-after respected");
    context.now=context.deadline;check(!exercise::retry(context,random),"deadline is shared across attempts");
    c11::token_bucket limiter(2,10ms,now);
    check(limiter.take(now)&&limiter.take(now)&&!limiter.take(now),"burst capacity enforced");
    check(!limiter.take(now+9999us)&&limiter.take(now+10ms),"integer refill does not grant early tokens");
    check(!limiter.take(now-1s),"backward observation does not mint tokens");
    check(limiter.take(now+1h,2)&&!limiter.take(now+1h),"long idle refill saturates at capacity");
    check(!limiter.take(now+1h,0),"zero cost rejected");
    std::cout<<"retry, idempotency budget, jitter and rate limiting passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 2;}}
