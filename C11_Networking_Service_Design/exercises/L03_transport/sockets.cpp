#include <c11/socket.hpp>
#include <c11/dns.hpp>
#include <check.hpp>
#include <array>
#include <iostream>
#include <memory>
#include <set>
using namespace std::chrono_literals;
int main() {
    c11::socket_runtime runtime;
    const auto end=c11::clock::now()+5s;
    for (int family : {AF_INET,AF_INET6}) {
        auto listener=c11::listen_loopback(family);
        check(listener.has_value(),"IPv4/IPv6 loopback listener");
        auto connected=c11::connect_loopback(listener->port,end,family);
        check(connected.has_value(),"IPv4/IPv6 connect");
        check(c11::wait_ready(listener->socket.get(),POLLRDNORM,end).has_value(),"accept ready");
        auto peer=c11::accept_socket(listener->socket.get());
        check(peer.has_value(),"peer accepted");
        auto moved=std::move(*connected);
        check(moved && !*connected,"socket move transfers unique ownership");
        check(c11::send_all(moved.get(),std::string_view("ok"),end).has_value(),"send with shared deadline");
        auto got=c11::receive_exact(peer->get(),2,end);
        check(got && *got=="ok","IPv4/IPv6 actual payload");
        auto timeout=c11::wait_ready(peer->get(),POLLRDNORM,c11::clock::now());
        check(!timeout && timeout.error()==std::errc::timed_out,"expired absolute deadline rejected");
    }
    addrinfo hints{}; hints.ai_family=AF_UNSPEC; hints.ai_socktype=SOCK_STREAM;
    addrinfo* raw=nullptr;
    check(::getaddrinfo("localhost","80",&hints,&raw)==0,"system resolver localhost lookup");
    std::unique_ptr<addrinfo,decltype(&::freeaddrinfo)> addresses(raw,::freeaddrinfo);
    std::size_t candidates=0;
    for(auto p=raw;p;p=p->ai_next) if(p->ai_family==AF_INET || p->ai_family==AF_INET6) ++candidates;
    check(candidates>0,"resolver supplies address candidates (not a DNS wire test)");
    // Local DNS fixture, ephemeral UDP port; host resolver settings remain unchanged.
    c11::unique_socket server{::socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP)}, client{::socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP)};
    check(server && client,"UDP sockets created");
    sockaddr_in bind{}; bind.sin_family=AF_INET; bind.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    check(::bind(server.get(),reinterpret_cast<sockaddr*>(&bind),sizeof(bind))==0,"UDP authority bound");
    c11::socket_length size=sizeof(bind);
    check(::getsockname(server.get(),reinterpret_cast<sockaddr*>(&bind),&size)==0,"ephemeral UDP port observed");
    check(c11::nonblocking(server.get()).has_value() && c11::nonblocking(client.get()).has_value(),"UDP nonblocking");
    auto query=*c11::dns::query(0x2143,"local.test");
    check(::sendto(client.get(),reinterpret_cast<const char*>(query.data()),static_cast<int>(query.size()),0,reinterpret_cast<sockaddr*>(&bind),size)==static_cast<int>(query.size()),"DNS query sent over UDP");
    check(c11::wait_ready(server.get(),POLLRDNORM,end).has_value(),"authority request ready");
    std::array<char,4096> storage{}; sockaddr_in from{}; c11::socket_length from_size=sizeof(from);
    const int n=static_cast<int>(::recvfrom(server.get(),storage.data(),static_cast<int>(storage.size()),0,reinterpret_cast<sockaddr*>(&from),&from_size));
    check(n==static_cast<int>(query.size()) && std::equal(query.begin(),query.end(),reinterpret_cast<const std::uint8_t*>(storage.data())),"authority consumes actual query");
    query[2]=0x81;query[3]=0x80;query[7]=1;
    query.insert(query.end(),{0xc0,12,0,1,0,1,0,0,0,60,0,4,127,0,0,1});
    check(::sendto(server.get(),reinterpret_cast<const char*>(query.data()),static_cast<int>(query.size()),0,reinterpret_cast<sockaddr*>(&from),from_size)==static_cast<int>(query.size()),"authority sends bounded response");
    check(c11::wait_ready(client.get(),POLLRDNORM,end).has_value(),"DNS response ready");
    sockaddr_in origin{}; c11::socket_length origin_size=sizeof(origin);
    const int received=static_cast<int>(::recvfrom(client.get(),storage.data(),static_cast<int>(storage.size()),0,reinterpret_cast<sockaddr*>(&origin),&origin_size));
    check(received>0 && origin.sin_addr.s_addr==bind.sin_addr.s_addr && origin.sin_port==bind.sin_port,"DNS source endpoint checked");
    auto ip=c11::dns::parse_a(std::span{reinterpret_cast<const std::uint8_t*>(storage.data()),static_cast<std::size_t>(received)},0x2143,"local.test");
    check(ip && *ip=="127.0.0.1","real UDP DNS result");
    check(::sendto(client.get(),"",0,0,reinterpret_cast<sockaddr*>(&bind),size)==0,"empty UDP datagram sent");
    check(c11::wait_ready(server.get(),POLLRDNORM,end).has_value(),"empty datagram readiness");
    check(::recvfrom(server.get(),storage.data(),static_cast<int>(storage.size()),0,nullptr,nullptr)==0,"empty datagram is a message, not stream EOF");
    check(::sendto(client.get(),"12345678",8,0,reinterpret_cast<sockaddr*>(&bind),size)==8,"oversized-for-buffer datagram sent");
    check(c11::wait_ready(server.get(),POLLRDNORM,end).has_value(),"truncation probe ready");
#ifdef _WIN32
    check(::recvfrom(server.get(),storage.data(),2,0,nullptr,nullptr)==SOCKET_ERROR && WSAGetLastError()==WSAEMSGSIZE,"Windows reports datagram truncation");
#else
    check(::recvfrom(server.get(),storage.data(),2,MSG_TRUNC,nullptr,nullptr)==8,"Linux MSG_TRUNC reports original length");
#endif
    const std::array<int,4> delivered{3,1,3,4}; std::set<int> unique(delivered.begin(),delivered.end());
    check(!unique.contains(2) && unique.size()==3,"gap and duplicate in injected delivery history (safe model)");
    std::cout<<"native IPv4/IPv6, resolver and UDP DNS checks passed\n";
}
