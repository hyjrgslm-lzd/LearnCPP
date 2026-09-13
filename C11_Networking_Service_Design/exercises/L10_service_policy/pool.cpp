#include <c11/socket_pool.hpp>
#include <c11/reactor.hpp>
#include <check.hpp>
#include <future>
#include <iostream>
using namespace std::chrono_literals;
int main() {
    c11::socket_runtime network;c11::reactor server;std::atomic_bool stop=false;bool drained=false;
    const auto end=c11::clock::now()+10s;
    std::jthread owner([&]{drained=server.run(stop,end);});
    c11::socket_pool pool(server.port(),1,1,100ms);
    for(const auto text:{"first","second"}) {
        auto connection=pool.acquire(end);check(connection.has_value(),"pool lease acquired");
        auto frame=*c11::encode_frame(text);check(c11::send_all(connection->get(),frame,end).has_value(),"pooled request sent");
        auto response=c11::receive_exact(connection->get(),frame.size(),end);check(response && *response==frame,"pooled response validated before recycling");connection->reusable();
    }
    check(pool.created()==1,"two actual requests reused one TCP connection");
    pool.expire_idle(c11::clock::now()+1h); // Explicit maintenance-time injection, not a sleep-based assertion.
    auto held=pool.acquire(end);check(held && pool.created()==2,"expired idle socket replaced");
    std::promise<std::error_code> timed_out;auto timeout_result=timed_out.get_future();
    std::jthread waiter([&]{auto r=pool.acquire(c11::clock::now()+500ms);timed_out.set_value(r?std::error_code{}:r.error());});
    check(pool.wait_for_waiters(1,end),"waiter is actually queued");
    auto refused=pool.acquire(end);check(!refused && refused.error()==std::errc::no_buffer_space,"bounded wait queue refuses excess admission");
    waiter.join();check(timeout_result.get()==std::errc::timed_out,"pool queue timeout uses absolute deadline");
    std::promise<std::error_code> stopped;auto stopped_result=stopped.get_future();
    std::jthread closing_waiter([&]{auto r=pool.acquire(end);stopped.set_value(r?std::error_code{}:r.error());});
    check(pool.wait_for_waiters(1,end),"close fixture waiter queued");
    check(!pool.close(),"pool close retains borrowed socket until lease is released");
    closing_waiter.join();check(stopped_result.get()==std::errc::operation_canceled,"pool close wakes waiting callers");
    held->reset();check(pool.close(),"last returned lease completes pool close");
    stop=true;owner.join();check(drained,"pool/server ownership converges");
    std::cout<<"real connection reuse, expiration, pool exhaustion, queue timeout and close passed\n";
}
