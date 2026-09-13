#include <c11/socket.hpp>
#include <c11/frame.hpp>
#include <rpc/client.hpp>
#include <check.hpp>
#include <future>
#include <iostream>
using namespace std::chrono_literals;
// Independent wire peer: no C09 parser or serializer on the server side.
int main() {
    c11::socket_runtime network;
    auto listener=c11::listen_loopback();check(bool(listener),"legacy listener");
    std::exception_ptr server_error;bool cancelled=false;
    std::jthread server([&]{try{
        const auto end=c11::clock::now()+5s;
        check(bool(c11::wait_ready(listener->socket.get(),POLLRDNORM,end)),"legacy accept ready");
        auto peer=c11::accept_socket(listener->socket.get());check(bool(peer),"legacy accepted");
        c11::frame_decoder decoder;
        const std::string expected[]={"Q|1|1|add|20,22","Q|2|0|missing|","Q|3|0|fail|","Q|4|0|slow|","C|4"};
        const std::string replies[]={"R|1|42|ok","R|2|0|unknown_method","R|3|0|error"};
        for(std::size_t i=0;i<5;++i){
            std::optional<std::string> body;
            while(!body){auto byte=c11::receive_exact(peer->get(),1,end);check(bool(byte),"legacy bytes");auto step=decoder.push((*byte)[0]);check(bool(step),"legacy frame");body=std::move(*step);}
            check(*body==expected[i],"unchanged C09 wire contract");
            if(i<3){auto wire=*c11::encode_frame(replies[i]);check(bool(c11::send_all(peer->get(),wire,end)),"legacy reply");}
            if(i==4)cancelled=true;
        }
    }catch(...){server_error=std::current_exception();}});
    asio::io_context io;rpc::RpcClient client(io);std::exception_ptr error;
    auto calls=[&]() -> asio::awaitable<void>{
        co_await client.connect("127.0.0.1",listener->port);
        auto sum=co_await client.call({0,"add",{20,22},true},1s);
        check(sum && sum->result==42,"legacy add");
        auto missing=co_await client.call({0,"missing",{},false},1s);
        check(!missing && missing.error()==rpc::RpcError::UnknownMethod,"legacy unknown method");
        auto failed=co_await client.call({0,"fail",{},false},1s);
        check(!failed && failed.error()==rpc::RpcError::ServerError,"legacy server error");
        auto timeout=co_await client.call({0,"slow",{},false},50ms);
        check(!timeout && timeout.error()==rpc::RpcError::Timeout,"legacy request timeout");
        // Do not close here: the peer closes only after observing C|4.
        // The client read loop remains work until that actual EOF is consumed.
    };
    asio::co_spawn(io,calls(),[&](std::exception_ptr e){error=e;if(e)client.shutdown();});io.run();client.shutdown();server.join();
    if(server_error)std::rethrow_exception(server_error);if(error)std::rethrow_exception(error);
    check(cancelled && client.in_flight()==0,"legacy cancel and lifetime convergence");
    std::cout<<"unchanged C09 client interoperates with independent C11 framed peer\n";
}
