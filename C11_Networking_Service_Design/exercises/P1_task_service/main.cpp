#include "client.hpp"
#include <check.hpp>
#include <iostream>
#include <thread>
using namespace c11::service;
using namespace std::chrono_literals;
int run_checks() {
    asio::io_context io;std::promise<void> gate;c11::tasks::limits limits;limits.live=2;
    auto host=std::make_shared<server>(io,limits,gate.get_future().share());host->start();
    std::exception_ptr error;std::jthread owner([&]{try{io.run();}catch(...){error=std::current_exception();}});
    struct cleanup {
        asio::io_context& io;std::shared_ptr<server> host;std::promise<void>& gate;std::jthread& owner;
        ~cleanup(){if(owner.joinable()){
            try{gate.set_value();}catch(const std::future_error&){}
            try{asio::post(io,[host=host]{host->stop();});}catch(...){io.stop();}
            owner.join();
        }}
    } cleanup_guard{io,host,gate,owner};
    auto health=request_once(host->port(),http::verb::get,"/health");check(health.result()==http::status::ok,"HTTP service healthy");
    const std::string body=R"({"left":20,"right":22,"steps":5,"budget_ms":5000})";
    auto created=request_once(host->port(),http::verb::post,"/tasks",body,"once");
    check(created.result()==http::status::accepted,"HTTP task admitted");
    const auto id=boost::json::parse(created.body()).at("id").to_number<std::uint64_t>();
    auto retry=request_once(host->port(),http::verb::post,"/tasks",body,"once");
    check(retry.result()==http::status::accepted && boost::json::parse(retry.body()).at("id").to_number<std::uint64_t>()==id,"response-loss retry returns same task");
    auto conflict=request_once(host->port(),http::verb::post,"/tasks",R"({"left":1,"right":2})","once");
    check(conflict.result()==http::status::conflict,"HTTP idempotency conflict");
    auto invalid=request_once(host->port(),http::verb::post,"/tasks",R"({"left":1e100,"right":2})","invalid");
    check(invalid.result()==http::status::bad_request,"JSON type/range checked before narrowing");
    auto second=request_once(host->port(),http::verb::post,"/tasks",body,"second");check(second.result()==http::status::accepted,"second gated task admitted");
    const auto second_id=boost::json::parse(second.body()).at("id").to_number<std::uint64_t>();
    auto overloaded=request_once(host->port(),http::verb::post,"/tasks",body,"third");check(overloaded.result()==http::status::too_many_requests,"HTTP overload rejected");
    asio::io_context websocket_io;websocket::stream<beast::tcp_stream> ws(websocket_io);
    beast::get_lowest_layer(ws).connect({asio::ip::address_v4::loopback(),host->port()});
    ws.handshake("localhost","/events?task_id="+std::to_string(id));
    beast::flat_buffer event;ws.read(event);
    check(boost::json::parse(beast::buffers_to_string(event.data())).at("state")=="running","WebSocket begins with atomic current snapshot");event.consume(event.size());
    auto canceled=request_once(host->port(),http::verb::delete_,"/tasks/"+std::to_string(id));check(canceled.result()==http::status::ok,"cancel endpoint accepted");
    auto still_full=request_once(host->port(),http::verb::post,"/tasks",body,"not-yet");check(still_full.result()==http::status::too_many_requests,"running cancel does not release physical capacity early");
    gate.set_value();bool terminal=false;std::uint64_t revision=0;
    for(int count=0;count<16 && !terminal;++count) {
        ws.read(event);auto message=boost::json::parse(beast::buffers_to_string(event.data()));event.consume(event.size());
        auto next=message.at("revision").to_number<std::uint64_t>();check(next>revision,"WebSocket state revisions ordered");revision=next;
        terminal=message.at("state")=="cancelled";
    }
    check(terminal,"WebSocket sends accepted cancellation terminal state");
    error_code closed;ws.read(event,closed);check(closed==websocket::error::closed,"WebSocket close handshake finishes after terminal event");
    const auto deadline=std::chrono::steady_clock::now()+3s;bool completed=false;
    while(std::chrono::steady_clock::now()<deadline && !completed) {
        auto result=request_once(host->port(),http::verb::get,"/tasks/"+std::to_string(second_id));
        const auto value=boost::json::parse(result.body());completed=value.at("state")=="succeeded";
        if(completed)check(value.at("result").to_number<int>()==42,"independent job completed with actual worker result");
    }
    check(completed,"job finishes within client total deadline");
    auto metrics=request_once(host->port(),http::verb::get,"/metrics");
    const auto totals=boost::json::parse(metrics.body());
    check(totals.at("accepted").to_number<unsigned>()==2 && totals.at("duplicates").to_number<unsigned>()==1,"metrics consume actual admission history");
    asio::post(io,[host]{host->stop();});owner.join();if(error)std::rethrow_exception(error);
    check(host->clean(),"HTTP, WebSocket, acceptor, timers and workers converge without dropped responses");
    std::cout<<"HTTP/JSON task service, WebSocket, idempotency, overload, cancellation and graceful stop passed\n";
    return 0;
}
int main(){try{return run_checks();}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 2;}}
