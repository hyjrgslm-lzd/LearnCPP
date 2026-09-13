#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <c11/certificate_fixture.hpp>
#include <check.hpp>
#include <iostream>
namespace asio=boost::asio;namespace ssl=asio::ssl;using tcp=asio::ip::tcp;
int main() {
    c11::certificates::authority ca;auto identity=ca.issue("localhost");
    asio::io_context io;ssl::context client_context(ssl::context::tls_client),server_context(ssl::context::tls_server);
    server_context.use_certificate_chain(asio::buffer(identity.certificate));
    server_context.use_private_key(asio::buffer(identity.key),ssl::context::pem);
    client_context.add_certificate_authority(asio::buffer(ca.certificate()));
    client_context.set_verify_mode(ssl::verify_peer);client_context.set_verify_callback(ssl::host_name_verification("localhost"));
    tcp::acceptor acceptor(io,{asio::ip::address_v4::loopback(),0});
    ssl::stream<tcp::socket> client(io,client_context),server(io,server_context);
    client.next_layer().connect(acceptor.local_endpoint());acceptor.accept(server.next_layer());
    check(SSL_set_tlsext_host_name(client.native_handle(),"localhost")==1,"TCP TLS SNI");
    const std::string payload="TLS on real TCP";std::string received(payload.size(),'\0'),reply(payload.size(),'\0');
    unsigned closes=0;
    server.async_handshake(ssl::stream_base::server,[&](boost::system::error_code e){
        check(!e,"server TCP TLS handshake");
        asio::async_read(server,asio::buffer(received),[&](boost::system::error_code error,std::size_t n){
            check(!error && n==payload.size() && received==payload,"real TCP TLS authenticated payload");
            asio::async_write(server,asio::buffer(received),[&](boost::system::error_code error,std::size_t count){
                check(!error && count==received.size(),"real TLS response sent");
                server.async_shutdown([&](boost::system::error_code error){check(!error,"server consumes TLS close");++closes;});
            });
        });
    });
    client.async_handshake(ssl::stream_base::client,[&](boost::system::error_code e){
        check(!e,"client TCP TLS verifies identity");
        asio::async_write(client,asio::buffer(payload),[&](boost::system::error_code error,std::size_t n){
            check(!error && n==payload.size(),"real TLS request sent");
            asio::async_read(client,asio::buffer(reply),[&](boost::system::error_code error,std::size_t count){
                check(!error && count==reply.size() && reply==payload,"real TLS response received");
                client.async_shutdown([&](boost::system::error_code error){check(!error,"client consumes TLS close");++closes;});
            });
        });
    });
    io.run();check(closes==2,"both real TCP TLS paths converge");std::cout<<"TCP TLS handshake, data and graceful close passed\n";
}
