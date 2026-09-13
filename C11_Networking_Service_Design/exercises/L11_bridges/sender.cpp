#include "network_sender.hpp"
#include <check.hpp>
#include <future>
#include <iostream>
struct receiver {
    using receiver_concept=stdexec::receiver_tag;
    struct env {std::stop_token token;std::stop_token query(stdexec::get_stop_token_t)const noexcept{return token;}};
    std::promise<int>* result;std::stop_token token;
    env get_env()const noexcept{return {token};}
    void set_value(std::string value)&&noexcept{result->set_value(value=="x"?1:4);}
    void set_error(std::exception_ptr)&&noexcept{result->set_value(2);}
    void set_stopped()&&noexcept{result->set_value(3);}
};
int main(){
    namespace asio=boost::asio;using tcp=asio::ip::tcp;
    asio::io_context io;auto work=asio::make_work_guard(io);std::jthread owner([&]{io.run();});
    struct cleanup{decltype(work)& w;std::jthread& t;~cleanup(){w.reset();t.join();}}guard{work,owner};
    for(int scenario=0;scenario<4;++scenario){
        tcp::acceptor acceptor(io,{asio::ip::address_v4::loopback(),0});
        auto client=std::make_shared<tcp::socket>(io);client->connect(acceptor.local_endpoint());tcp::socket peer(io);acceptor.accept(peer);
        std::promise<int> result;auto future=result.get_future();std::stop_source stop;
        if(scenario==3)stop.request_stop();
        auto op=stdexec::connect(c11::receive_sender{client},receiver{&result,stop.get_token()});stdexec::start(op);
        if(scenario==0)asio::write(peer,asio::buffer("x",1));
        if(scenario==1)peer.close();
        if(scenario==2)stop.request_stop();
        check(future.wait_for(std::chrono::seconds(4))==std::future_status::ready,"sender completes within budget");
        check(future.get()==(scenario==0?1:scenario==1?2:3),"network value/error/stopped channels");
    }
    // Stop arrives before the owner runs, but the socket was never open:
    // ordinary bad_descriptor must remain error, not become stopped.
    asio::io_context isolated;auto closed=std::make_shared<tcp::socket>(isolated);
    std::promise<int> ordinary;auto outcome=ordinary.get_future();std::stop_source late;
    auto failed=stdexec::connect(c11::receive_sender{closed},receiver{&ordinary,late.get_token()});
    stdexec::start(failed);late.request_stop();isolated.run();
    check(outcome.get()==2,"ordinary error survives a concurrent stop request");
    std::cout<<"real socket sender value, EOF error, active/pre-start cancellation passed\n";
}
