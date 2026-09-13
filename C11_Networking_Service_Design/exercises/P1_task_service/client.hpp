#pragma once
#include "server.hpp"
namespace c11::service {
inline asio::awaitable<response> async_request(std::uint16_t port,http::verb method,std::string path,std::string body,std::string key) {
    beast::tcp_stream stream(co_await asio::this_coro::executor);
    stream.expires_after(std::chrono::seconds(3));
    co_await stream.async_connect({asio::ip::address_v4::loopback(),port},asio::use_awaitable);
    request req(method,path,11);req.set(http::field::host,"localhost");req.keep_alive(false);
    if(!key.empty())req.set("Idempotency-Key",key);
    if(!body.empty()){req.set(http::field::content_type,"application/json");req.body()=std::move(body);req.prepare_payload();}
    co_await http::async_write(stream,req,asio::use_awaitable);
    beast::flat_buffer buffer(64*1024);http::response_parser<http::string_body> parser;parser.header_limit(8192);parser.body_limit(4096);
    co_await http::async_read(stream,buffer,parser,asio::use_awaitable);
    co_return parser.release();
}
inline response request_once(std::uint16_t port,http::verb method,std::string path,std::string body={},std::string key={}) {
    asio::io_context io;
    auto result=asio::co_spawn(io,async_request(port,method,std::move(path),std::move(body),std::move(key)),asio::use_future);
    io.run();return result.get();
}
}
