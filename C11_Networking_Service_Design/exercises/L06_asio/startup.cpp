#include <atomic>
#include <cstdlib>
#include <new>
namespace {std::atomic_bool fail_next_allocation=false;}
void* operator new(std::size_t size) {
    if(fail_next_allocation.exchange(false))throw std::bad_alloc();
    if(void* p=std::malloc(size?size:1))return p;
    throw std::bad_alloc();
}
void operator delete(void* p)noexcept{std::free(p);}
void operator delete(void* p,std::size_t)noexcept{std::free(p);}
#include <c11/task_runtime.hpp>
#include <c11/task_policy.hpp>
#include <check.hpp>
#include <iostream>
using runtime=c11::tasks::runtime<c11::tasks::task_policy>;
using namespace std::chrono_literals;
int main() {
    {
        boost::asio::io_context io;auto service=std::make_shared<runtime>(io);
        fail_next_allocation=true;bool failed=false;
        try{service->start();}catch(const std::bad_alloc&){failed=true;}
        fail_next_allocation=false;check(failed,"allocation failure injected during worker startup");
        service->start();bool drained=false;service->stop([&]{drained=true;});
        check(!drained && !service->joined(),"retried startup does not inherit earlier joined state");
        io.run();check(drained && service->joined() && io.stopped(),"retried runtime actually stops workers and timer");
    }
    {
        boost::asio::io_context io;auto service=std::make_shared<runtime>(io);service->start();
        bool drained=false;service->stop([&]{drained=true;});
        // Deliberately hold the owner, independent of worker scheduling. The
        // shutdown budget includes owner dispatch and join, even with no tasks.
        std::this_thread::sleep_for(5200ms);
        io.run();check(drained && service->joined() && service->deadline_exceeded,"late owner cannot disguise exceeded total shutdown budget");
    }
    std::cout<<"startup-failure retry and delayed-owner deadline regressions passed\n";
}
