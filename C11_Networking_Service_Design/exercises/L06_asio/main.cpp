#include <boost/asio.hpp>
#include <c11/frame.hpp>
#include <check.hpp>
#include <array>
#include <atomic>
#include <iostream>
#include <thread>
namespace asio=boost::asio;
using tcp=asio::ip::tcp;

asio::awaitable<void> echo_frame(tcp::socket socket) {
    c11::frame_decoder parser;
    std::array<char,8> header{};
    co_await asio::async_read(socket,asio::buffer(header),asio::use_awaitable);
    std::optional<std::string> body;
    for(char c:header) {
        auto step=parser.push(c);if(!step)throw std::runtime_error("invalid frame header");if(*step)body=std::move(**step);
    }
    while(!body) {
        char byte{};
        co_await asio::async_read(socket,asio::buffer(&byte,1),asio::use_awaitable);
        auto step=parser.push(byte);if(!step)throw std::runtime_error("invalid frame body");if(*step)body=std::move(**step);
    }
    auto response=*c11::encode_frame(*body);
    co_await asio::async_write(socket,asio::buffer(response),asio::use_awaitable);
    boost::system::error_code ignored;socket.shutdown(tcp::socket::shutdown_send,ignored);
}
int main() {
    asio::io_context io;
    tcp::acceptor listener(io,{asio::ip::address_v4::loopback(),0});
    tcp::socket client(io),peer(io);client.connect(listener.local_endpoint());listener.accept(peer);
    asio::cancellation_signal signal;std::array<char,8> canceled_buffer{};bool canceled=false;
    peer.async_read_some(asio::buffer(canceled_buffer),asio::bind_cancellation_slot(signal.slot(),[&](boost::system::error_code error,std::size_t bytes){
        check(error==asio::error::operation_aborted && bytes==0,"cancellation completes idle read without borrowing buffer afterwards");canceled=true;
    }));
    signal.emit(asio::cancellation_type::terminal);io.run();check(canceled,"cancellation handler actually ran");
    io.restart();std::exception_ptr error;
    asio::co_spawn(io,echo_frame(std::move(peer)),[&](std::exception_ptr e){error=e;});
    const auto input=*c11::encode_frame("awaitable");std::string response(input.size(),'\0');bool wrote=false,read=false;
    asio::async_write(client,asio::buffer(input),[&](boost::system::error_code e,std::size_t n){check(!e && n==input.size(),"composed write complete");wrote=true;});
    asio::async_read(client,asio::buffer(response),[&](boost::system::error_code e,std::size_t n){check(!e && n==response.size(),"composed read complete");read=true;});
    io.run();if(error)std::rethrow_exception(error);
    check(wrote && read && input==response,"co_spawn owns frame/buffers through real IO completion");
    asio::io_context shared;
    auto strand=asio::make_strand(shared);std::atomic_int inside=0;int count=0;
    for(int i=0;i<100;++i)asio::post(strand,[&]{check(inside.fetch_add(1)==0,"strand prevents overlapping handlers");++count;inside.fetch_sub(1);});
    std::jthread other([&]{shared.run();});shared.run();other.join();
    check(count==100,"two io_context threads consume serialized strand work");
    std::cout<<"Asio cancellation, coroutine IO and strand checks passed\n";
}
