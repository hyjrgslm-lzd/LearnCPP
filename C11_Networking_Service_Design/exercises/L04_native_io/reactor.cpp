#include <c11/reactor.hpp>
#include <check.hpp>
#include <iostream>
#include <thread>
using namespace std::chrono_literals;
int main() {
    c11::socket_runtime runtime;
    c11::reactor server(2048);
    std::atomic_bool stop=false;
    bool drained=false;std::exception_ptr server_error;
    const auto end=c11::clock::now()+20s;
    std::jthread owner([&]{try{drained=server.run(stop,end);}catch(...){server_error=std::current_exception();}});
    auto slow=c11::connect_loopback(server.port(),end),fast=c11::connect_loopback(server.port(),end);
    check(slow && fast,"two clients connected");
    const int receive_buffer=4096;
    check(::setsockopt(slow->get(),SOL_SOCKET,SO_RCVBUF,reinterpret_cast<const char*>(&receive_buffer),sizeof(receive_buffer))==0,"slow receive buffer requested");
    const auto frame=*c11::encode_frame(std::string(4088,'s'));
    constexpr std::size_t count=128;
    bool producer_ok=false;
    std::jthread producer([&]{
        producer_ok=true;
        for(std::size_t i=0;i<count;++i) if(!c11::send_all(slow->get(),frame,end)) {producer_ok=false;break;}
        if(producer_ok) producer_ok=c11::shutdown_write(slow->get()).has_value();
    });
    check(server.wait_for_backpressure(end),"real slow peer reaches application high water before reads resume");
    const auto request=*c11::encode_frame("fast peer");
    check(c11::send_all(fast->get(),request,end).has_value(),"fast request sent during slow-peer backpressure");
    auto response=c11::receive_exact(fast->get(),request.size(),end);
    check(response && *response==request,"other real connection progresses while slow reader is paused");
    check(c11::shutdown_write(fast->get()).has_value(),"fast half-close");
    for(std::size_t i=0;i<count;++i) {
        auto r=c11::receive_exact(slow->get(),frame.size(),end);
        check(r && *r==frame,"bounded slow responses drain without loss or reordering");
    }
    producer.join();check(producer_ok,"producer ends within shared deadline");
    for(auto socket:{slow->get(),fast->get()}) {
        check(c11::wait_ready(socket,POLLRDNORM,end).has_value(),"drained peer EOF ready");
        char c{};auto r=c11::read_some(socket,std::span{&c,1});check(r && *r==0,"drained EOF");
    }
    stop=true;owner.join();
    if(server_error) std::rethrow_exception(server_error);
    check(drained && server.accepted==2 && server.protocol_errors==0,"reactor closes all peers");
    check(server.read_bytes==count*frame.size()+request.size() && server.written_bytes==server.read_bytes,"real transfer accounting");
    // Reproduce abandoned accepted output through the real public server API.
    c11::reactor interrupted(2048);std::atomic_bool stop_bad=false,stop_producer=false;
    bool incorrectly_clean=true;std::exception_ptr failure;
    const auto failed_end=c11::clock::now()+10s;
    std::jthread second_owner([&]{try{incorrectly_clean=interrupted.run(stop_bad,failed_end);}catch(...){failure=std::current_exception();}});
    auto stalled=c11::connect_loopback(interrupted.port(),failed_end);check(stalled.has_value(),"failure peer connects");
    check(::setsockopt(stalled->get(),SOL_SOCKET,SO_RCVBUF,reinterpret_cast<const char*>(&receive_buffer),sizeof(receive_buffer))==0,"failure peer receive limit");
    const auto handle=stalled->get();
    std::jthread flood([&]{
        std::size_t sent=0;
        while(!stop_producer && c11::clock::now()<failed_end && sent<2*1024*1024) {
            auto bytes=std::span<const char>{frame}.subspan(sent%frame.size());
            auto n=c11::write_some(handle,bytes);
            if(n && *n)sent+=*n;
            else if(!n && c11::would_block(n.error().value())) {
                c11::poll_fd fd{handle,POLLWRNORM,0};(void)c11::poll_sockets(std::span{&fd,1},10);
            }else break;
        }
    });
    check(interrupted.wait_for_backpressure(failed_end),"failure fixture has accepted pending responses");
    stop_producer=true;flood.join(); // Join before closing the socket owned by this thread.
    stop_bad=true;
    linger reset{1,0};check(::setsockopt(stalled->get(),SOL_SOCKET,SO_LINGER,reinterpret_cast<const char*>(&reset),sizeof(reset))==0,"request reset on close");
    stalled->reset();second_owner.join();
    if(failure)std::rethrow_exception(failure);
    check(!incorrectly_clean && interrupted.abandoned_connections>0,"discarded pending responses cannot report clean drain");
    std::cout<<"bounded reactor and real slow-reader fairness passed; kernel buffer request="<<receive_buffer<<'\n';
}
