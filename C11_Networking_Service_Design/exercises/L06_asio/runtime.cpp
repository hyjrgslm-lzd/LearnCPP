#include <c11/task_runtime.hpp>
#include <c11/task_policy.hpp>
#include <check.hpp>
#include <iostream>
using namespace std::chrono_literals;
namespace asio=boost::asio;
using runtime=c11::tasks::runtime<c11::tasks::task_policy>;
int main() {
    {
        asio::io_context io;auto service=std::make_shared<runtime>(io);service->start();
        auto task=service->submit({20,22,0,5000},"normal");check(task.has_value(),"runtime task accepted");
        bool drained=false;service->stop([&]{drained=true;});io.run();
        auto final=service->registry.get(task->task,c11::tasks::clock::now());
        check(final && final->phase==c11::tasks::state::succeeded && final->value==42,"worker publishes result through fixed completion slot");
        check(drained && service->joined() && !service->failure && !service->deadline_exceeded,"normal runtime drain joins workers");
    }
    {
        asio::io_context io;std::promise<void> gate;
        c11::tasks::limits bound;bound.live=1;
        auto service=std::make_shared<runtime>(io,bound,gate.get_future().share());service->start();
        auto task=service->submit({1,2,1,60000},"gated");check(task.has_value(),"gated task accepted");
        check(service->registry.cancel(task->task,c11::tasks::clock::now()).has_value(),"cancel requested before worker acknowledgement");
        check(service->registry.live()==1 && !service->submit({1,2,0,5000},"other"),"physical work quota remains reserved");
        bool drained=false,overdue_observed=false;service->stop([&]{drained=true;});
        asio::steady_timer controller(io);controller.expires_after(5200ms);
        controller.async_wait([&](boost::system::error_code error){
            check(!error,"controlled deadline observer runs");
            overdue_observed=service->deadline_exceeded && service->registry.live()==1 && !drained;
            gate.set_value(); // Only this event permits the physical worker to acknowledge.
        });
        io.run();
        check(overdue_observed,"exceeding shutdown budget is reported before cleanup completes");
        check(drained && service->joined() && service->registry.live()==0 && service->deadline_exceeded,"late cleanup does not turn an exceeded deadline into success");
        check(service->registry.get(task->task,c11::tasks::clock::now())->phase==c11::tasks::state::cancelled,"late worker result cannot overwrite cancellation");
    }
    std::cout<<"runtime fixed-slot ownership and explicit shutdown-deadline failure checks passed\n";
}
